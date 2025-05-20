
#pragma once
#include <atomic>
#include <mutex>
#include <vector>
#include <memory>

class CombiningTree {
public: 
    enum class Status{
        Idle = 0,
        First = 1,
        Second = 2,
        Result = 3,
        Root = 4,
    };

    class Node{
    public: 
        int first_;
        int second_;
        int result_;
        Node* parent_;
        Status status_;
        std::atomic<bool> locked_;
    
        Node(Status s, Node* p, int backoff): locked_(false), status_(s), parent_(p), result_(0), second_(0), first_(0), micro_backoff_(backoff) {}
        bool precombine();
        int combine(int accumulate);
        int op(int accumulate);
        void distribute(int prior);

    private:
        std::mutex mtx_precomb_;
        std::mutex mtx_comb_;
        std::mutex mtx_op_;
        std::mutex mtx_dist_;
        int micro_backoff_;
    
        void spin_lock_for_access();
    };

    int width_;
    std::vector<std::unique_ptr<Node>> nodes_;
    std::vector<Node*> leaves_;
    
    CombiningTree(int width, int backoff);
    int get_and_increment(int id);

};

