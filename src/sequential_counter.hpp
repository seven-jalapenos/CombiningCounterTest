
#pragma once
#include <mutex>

class SequentialCounter{
private:
    std::mutex mtx_;
    size_t count_ = 0;

public:
    int get_and_increment();
};