
#pragma once
#include <mutex>
#include <atomic>

struct Spinlock{
    void lock();
    void unlock();

private:
    std::atomic<bool> locked{false};
};

class SequentialCounter{
private:
    Spinlock lock;
    size_t count_ = 0;

public:
    int get_and_increment();
};