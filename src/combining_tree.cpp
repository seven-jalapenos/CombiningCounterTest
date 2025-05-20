
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <stdexcept>
#include <thread>

#include "combining_tree.hpp"

constexpr auto Idle =  CombiningTree::Status::Idle;
constexpr auto First = CombiningTree::Status::First;
constexpr auto Second = CombiningTree::Status::Second;
constexpr auto Result = CombiningTree::Status::Result;
constexpr auto Root = CombiningTree::Status::Root;

// std::string toInt(CombiningTree::Status){
// return static_cast<std::underlying_type<CombiningTree::Status>int>()
// }

CombiningTree::CombiningTree(int width, int backoff = 10){
    if(width < 1){
        width = 2;
    }
    
    // round to next highest power of two
    int rounded = 2;
    while(rounded < width){
        rounded <<= 1;
    }
    width = rounded;
    width_ = width;
    leaves_.resize(width/2);// half of width because we rounded
    nodes_.resize(leaves_.size() * 2 - 1);

    nodes_[0] = std::make_unique<Node>(Root, nullptr, backoff);
    for (size_t i = 1; i < nodes_.size(); ++i) {
        nodes_[i] = std::make_unique<Node>(Idle, nodes_[(i - 1) / 2].get(), backoff);
    }
    
    size_t offset = nodes_.size() - leaves_.size();
    for (size_t i = 0; i < leaves_.size(); ++i) {
        leaves_[i] = nodes_[offset + i].get();
    }
}

void CombiningTree::Node::spin_lock_for_access(){
    // used to coordinate handoff between active and passive thread
    bool test_locked = false;
    while(!locked_.compare_exchange_weak(test_locked, true)){
        test_locked = false;
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

bool CombiningTree::Node::precombine(){
    // will return true as long as thread can progress
    // use to find stop node for thread

    std::lock_guard<std::mutex> lck (mtx_precomb_);
    spin_lock_for_access();

    switch(status_){
    case Idle:
        status_ = First;
        locked_.store(false);
        return true;
    case First:
        status_ = Second;
        locked_.store(true);
        return false;
    case Root:
        locked_.store(false);
        return false;
    default:
        std::cout << static_cast<int>(status_) << std::endl;
        throw(std::runtime_error("unexpected status during precombine stage\n"));
    }

    return false;
}

int CombiningTree::Node::combine(int accumulate){
    // pass current value along if active thread
    // if passive thread, get stored value and current value

    std::lock_guard<std::mutex> lck (mtx_comb_);
    spin_lock_for_access();

    first_ = accumulate;
    switch(status_){
    case First:
        return first_;
    case Second:
        return first_ + second_;
    default:
        throw(std::runtime_error("unexpected status during combine stage\n"));
    }

    return -1;
}

int CombiningTree::Node::op(int accumulate){
    std::lock_guard<std::mutex> lck (mtx_op_);

    switch(status_){
    case Second:{
        second_ = accumulate;
        locked_.store(false);
        while(status_ != Result){
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        status_ = Idle;
        locked_.store(false);
        return result_;
    }
    case Root:{
        int prior = result_;    
        result_ += accumulate;
        return prior;
    }
    default:
        throw(std::runtime_error("unexpected status during op stage\n"));    
    }
    return -1;
}

void CombiningTree::Node::distribute(int prior){
    // reset node if there is no passive thread
    // deposit value and signal if passive thread is waiting

    std::lock_guard<std::mutex> lck (mtx_dist_);

    switch(status_){
    case First:
        status_ = Idle;
        locked_.store(false);
        break;
    case Second:
        result_ = prior + first_;
        status_ = Result;
        break;
    default:
        throw(std::runtime_error("unexpected status during distribute phase\n"));
    }
}

int CombiningTree::get_and_increment(int id){
    if(id >= width_ || id < 0){
        throw(std::runtime_error("invalid id value"));
    }

    // thread start at leaf
    Node* start = leaves_[id/2];
    Node* n = start;
    std::vector<Node*> path;

    // precombine phase
    // start at leaf
    // end at node with active thread or root
    while(n->precombine()){
        path.push_back(n);
        n = n->parent_;
    }
    Node* stop = n;

    // combine phase
    // deposit current request at node
    // accumulate requests along path
    int accumulate = 1;
    n = start;
    while(n != stop){
        accumulate = n->combine(accumulate);
        n = n->parent_;
    }

    // op phase
    // retrieve counter value if root
    // deposit requests at node
    // if passive thread, wait for active thread to deposit value
    int prior = stop->op(accumulate);

    // distribute phase
    // reset if no passive thread
    // calculate result if passive thread
    while(!path.empty()){
        n = path.back();
        path.pop_back();
        n->distribute(prior);
    }
    
    return prior;
}