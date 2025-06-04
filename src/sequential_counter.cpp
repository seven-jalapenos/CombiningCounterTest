
#include <mutex>
// #include <iostream>
#include "sequential_counter.hpp"
#include <xmmintrin.h>

void Spinlock::lock(){
    for(;;){
        bool was_locked = locked.load(std::memory_order_relaxed);
        if(!was_locked && locked.compare_exchange_weak(was_locked, true, std::memory_order_acquire)){
            break;
        }
        _mm_pause();
    }
}

void Spinlock::unlock(){
    locked.store(false, std::memory_order_release);
}

int SequentialCounter::get_and_increment(){
    lock.lock();
    int prior = count_;
    count_++;
    lock.unlock();
    // std::cout << prior << std::endl; // for debugging
    return prior;
}