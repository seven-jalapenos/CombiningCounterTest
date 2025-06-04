
#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <cassert>


#include "combining_tree.hpp"
#include "sequential_counter.hpp"

int seive(int work){
    int n = work;
    std::vector<bool> prime(n, true);
    for (int p = 2; p * p < n; ++p) {
        if (prime[p]) {
            for (int i = p * p; i < n; i += p)
                prime[i] = false;
        }
    }
    int num = 0;
    std::vector<int> res(n, -1);
    for (size_t i = 0; i < prime.size(); i++)
    {
        if (prime[i]){
            res.push_back(i);
            num++;
        }
    }
    res.resize(num);
    return res.size();
}

int main(int argc, char* argv[]){
    // std::mutex io_mtx;
    // size_t global_thread_id = 0;
    int num_processor = 8;
    int work = 100;
    int runs = 20;
    int op_per_thread = 2048;
    int micro_backoff = 10;
    int thread_per_processor = 3;
    int warm_ups = 3;
    switch (argc) {
    case 7:
        thread_per_processor = std::stoi(argv[6]);
        [[fallthrough]];
    case 6:
        micro_backoff = std::stoi(argv[5]);
        [[fallthrough]];
    case 5:
        op_per_thread = std::stoi(argv[4]);
        [[fallthrough]];
    case 4:
        runs = std::stoi(argv[3]);
        [[fallthrough]];
    case 3:
     work = std::stoi(argv[2]);
        [[fallthrough]];
    case 2:
        num_processor = std::stoi(argv[1]);
        [[fallthrough]];
    case 1:
        break;
    default:
        throw(std::runtime_error("invalid number of inline arguments"));
    }

    int num_thread = num_processor * thread_per_processor;

    std::vector<std::thread> t(num_thread);
    std::vector<int> rets(num_thread * op_per_thread, -1);
    std::vector<int> seq_rets(num_thread * op_per_thread, -1);
    // for error checking ^^

    auto set_cpu = [=](std::thread& t, int id){
        int core_i = id / thread_per_processor;
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(core_i, &set);
        int rc = pthread_setaffinity_np(t.native_handle(), sizeof(set), &set);
        if(rc != 0){
            std::cerr << "error calling pthread_setaffinity_np: " << rc << std::endl;
        }
        // return 0;
    };

    auto combining_increment = [&, work](int id, std::vector<int>& arr, CombiningTree& counter, auto work_time)->void{
        int res;
        for (int j = 0; j < op_per_thread; j++) {
            arr[id * op_per_thread + j] = counter.get_and_increment(id);
            auto start = std::chrono::high_resolution_clock::now();
            res = seive(work);
            auto end = std::chrono::high_resolution_clock::now();
            work_time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        }
        // std::lock_guard<std::mutex> lock(io_mtx);
        // std::cout << "combinc thread #" << global_thread_id++ << " running on cpu " << sched_getcpu() << "\n";
    };

    auto sequential_increment = [&, work](int id, std::vector<int>& arr, SequentialCounter& counter, auto work_time)->void{
        int res;
        for (int j = 0; j < op_per_thread; j++) {
            arr[id * op_per_thread + j] = counter.get_and_increment();
            auto start = std::chrono::high_resolution_clock::now();
            res = seive(work);
            auto end = std::chrono::high_resolution_clock::now();
            work_time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        }
        // std::lock_guard<std::mutex> lock(io_mtx);
        // std::cout << "seqinc thread #" << global_thread_id++ << " running on cpu " << sched_getcpu() << "\n";
    };

    auto run_increment = [=](int runs, std::vector<std::thread>& t, std::vector<int>& arr)->auto{
        std::chrono::microseconds total(0);
        for (int j = 0; j < runs; j++) {
            int thread_id = 0;
            CombiningTree counter(num_thread, micro_backoff);
            std::chrono::nanoseconds work_time(0);
            auto start = std::chrono::high_resolution_clock::now();
            for (auto& i : t) {
                try{
                    i = std::thread(combining_increment, thread_id, std::ref(arr), std::ref(counter), work_time);
                    set_cpu(i, thread_id);
                    thread_id++;
                } catch (const std::exception& e) {
                    std::cerr << "Thread " << thread_id << " threw exception " << e.what() << "\n";
                } catch (...){
                    std::cerr << "Thread " << thread_id << " threw unknown exception" << "\n"; 
                }
            }
            for (auto& i : t) {
                i.join();
            }
            auto end = std::chrono::high_resolution_clock::now();
            total += std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            total -= std::chrono::duration_cast<std::chrono::microseconds>(work_time);
            std::sort(arr.begin(), arr.end());
            assert(arr.back() == num_thread * op_per_thread - 1);
            std::fill(arr.begin(), arr.end(), -1);
        }
        // average for each operation
        double totalf = std::chrono::duration<double, std::micro>(total).count();
        double avg = totalf / (runs * num_thread * op_per_thread);
        return avg;
    };

    auto run_increment2 = [=](int runs, std::vector<std::thread>& t, std::vector<int>& arr)->auto{
        std::chrono::microseconds total(0);
        for (int j = 0; j < runs; j++) {
            int thread_id = 0;
            SequentialCounter counter;
            std::chrono::nanoseconds work_time(0);
            auto start = std::chrono::high_resolution_clock::now();
            for (auto& i : t) {
                try{
                    i = std::thread(sequential_increment, thread_id, std::ref(arr), std::ref(counter), work_time);
                    set_cpu(i, thread_id);
                    thread_id++;
                } catch (const std::exception& e) {
                    std::cerr << "Thread " << thread_id << " threw exception " << e.what() << "\n";
                } catch (...){
                    std::cerr << "Thread " << thread_id << " threw unknown exception" << "\n"; 
                }
            }
            for (auto& i : t) {
                i.join();
            }
            auto end = std::chrono::high_resolution_clock::now();
            total += std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            total -= std::chrono::duration_cast<std::chrono::microseconds>(work_time);
            std::sort(arr.begin(), arr.end());
            assert(arr.back() == num_thread * op_per_thread - 1);
            std::fill(arr.begin(), arr.end(), -1);
        }
        // average for each operation
        double totalf = std::chrono::duration<double, std::micro>(total).count();
        double avg = totalf / (runs * num_thread * op_per_thread);
        return avg;
    };


    run_increment(warm_ups, std::ref(t), std::ref(rets));
    auto combined_avg = run_increment(runs, std::ref(t), std::ref(rets));

    run_increment2(warm_ups, std::ref(t), std::ref(seq_rets));
    auto seq_combined_avg = run_increment2(runs, std::ref(t), std::ref(seq_rets));

    std::cout << combined_avg << " " << seq_combined_avg;
    // std::cout << "\n" << "global thread id: " << global_thread_id << " theoretically correct threads " << ((warm_ups * num_thread) + (runs * num_thread)) * 2 << "\n";

    return 0;
}