
#include <mutex>
// #include <iostream>
#include "sequential_counter.hpp"

int SequentialCounter::get_and_increment(){
    std::lock_guard<std::mutex> lck(mtx_);
    int prior = count_;
    count_++;
    // std::cout << prior << std::endl;
    return prior;
}