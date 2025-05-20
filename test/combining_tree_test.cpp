
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

// struct CombiningFunctor{
//     int id_;
//     CombiningTree& ct_;

//     CombiningFunctor(CombiningTree& ct): id_(0), ct_(ct) {}

//     void operator()(){

//     }
// }

int main(int argc, char* argv[]){
    int num_threads = 8;
    int contention_mill = 0;
    int runs = 20;
    int op_per_thread = 2;
    int micro_backoff = 10;
    int warm_ups = 3;
    switch (argc) {
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
        contention_mill = std::stoi(argv[2]);
        [[fallthrough]];
    case 2:
        num_threads = std::stoi(argv[1]);
        [[fallthrough]];
    case 1:
        break;
    default:
        throw(std::runtime_error("invalid number of inline arguments"));
    }

    std::vector<std::thread> t(num_threads);
    std::vector<int> rets(num_threads * op_per_thread, -1);
    std::vector<int> seq_rets(num_threads * op_per_thread, -1);

    auto combining_increment = [&, contention_mill](int id, std::vector<int>& arr, CombiningTree& counter)->void{
        for (int j = 0; j < op_per_thread; j++) {
            arr[id * op_per_thread + j] = counter.get_and_increment(id);
            std::this_thread::sleep_for(std::chrono::milliseconds(contention_mill));
        }
    };

    auto run_combining_increment = [=](int runs, std::vector<std::thread>& t, std::vector<int>& arr)->auto{
        std::chrono::microseconds total(0);
        for (int j = 0; j < runs; j++) {
            int thread_id = 0;
            CombiningTree counter(num_threads, micro_backoff);
            auto start = std::chrono::high_resolution_clock::now();
            for (auto& i : t) {
                i = std::thread( combining_increment, thread_id, std::ref(arr), std::ref(counter));
                thread_id++;
            }
            for (auto& i : t) {
                i.join();
            }
            auto end = std::chrono::high_resolution_clock::now();
            total += std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            std::sort(arr.begin(), arr.end());
            assert(arr.back() == num_threads * op_per_thread - 1);
            std::fill(arr.begin(), arr.end(), -1);
        }
        auto avg = total/runs;
        return avg;
    };

    auto sequential_increment = [&, contention_mill](int id, std::vector<int>& arr, SequentialCounter& counter)->void{
        for (int j = 0; j < op_per_thread; j++) {
            arr[id * op_per_thread + j] = counter.get_and_increment();
            std::this_thread::sleep_for(std::chrono::milliseconds(contention_mill));
        }
    };

    auto run_sequential_increment = [=](int runs, std::vector<std::thread>& t, std::vector<int>& arr)->auto{
        std::chrono::microseconds total(0);
        for (int j = 0; j < runs; j++) {
            SequentialCounter counter;
            int thread_id = 0;
            auto start = std::chrono::high_resolution_clock::now();
            for (auto& i : t) {
                i = std::thread(sequential_increment, thread_id, std::ref(arr), std::ref(counter));
                thread_id++;
            }
            for (auto& i : t) {
                i.join();
            }
            auto end = std::chrono::high_resolution_clock::now();
            total += std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            // std::for_each(arr.begin(), arr.end(), [](int a){std::cout << a << " ";});
            // std::cout << std::endl;
            std::sort(arr.begin(), arr.end());
            assert(arr.back() == num_threads * op_per_thread - 1);
            std::fill(arr.begin(), arr.end(), -1);
        }
        auto avg = total/runs;
        return avg;
    };

    // for(int i = 0; i < warm_ups; i++){
    //     int thread_id = 0;
    //     for (auto& j : t) {
    //         j = std::thread(increment, thread_id, std::ref(rets));
    //         thread_id++;
    //     }
    //     for (auto& j : t) {
    //         j.join();
    //     }
    // }

    run_combining_increment(warm_ups, std::ref(t), std::ref(rets));
    auto combined_avg = run_combining_increment(runs, std::ref(t), std::ref(rets));

    run_sequential_increment(warm_ups, std::ref(t), std::ref(seq_rets));
    auto seq_combined_avg = run_sequential_increment(warm_ups, std::ref(t), std::ref(seq_rets));

    std::cout << combined_avg.count() << " " << seq_combined_avg.count();

    // std::sort( rets.begin(), rets.end() );
    // std::cout << "ordered previous return vals: ";
    // std::for_each( rets.begin(), rets.end(), [&](int a){ std::cout << a << " "; } );
    // std::cout << std::endl;

    return 0;
}