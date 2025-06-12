#include <iostream>
#include "btree_olc.h"
#include "coroutine/coroutine_round_robin_executor.h"
#include <sstream>
#include <fstream>
#include <chrono>
#include <filesystem>

#include "PerfEvent.hpp"

int main() {
    auto tree = BTree<std::uint64_t, std::uint64_t>{};

    /// Create the workload.
    constexpr auto insert_requests = 50000000ULL;
    constexpr auto lookup_requests = 50000000ULL;
    auto benchmark_set = NumericWorkloadSet{insert_requests, lookup_requests};
    
    /// Execute the insert_requests phase.
    std::cout << "Executing " << insert_requests << " insert_requests requests..." << std::endl;
    {
        PerfEvent e;
        e.startCounters();
        CoroutineRoundRobinExecutor::execute(tree, benchmark_set.insert_requests());
        e.stopCounters();
        e.printReport(std::cout, insert_requests); // use insert_requests as scale factor
    }
    std::cout << "done" << std::endl;

    /// Execute the lookup phase.
    std::cout << "\nExecuting " << lookup_requests << " lookup requests..." << std::endl;
    const auto start_timestamp = std::chrono::steady_clock::now();
    {
        PerfEvent e;
        e.startCounters();
        CoroutineRoundRobinExecutor::execute(tree, benchmark_set.mixed_requests());
        e.stopCounters();
        e.printReport(std::cout, lookup_requests); // use lookup_requests as scale factor
    }
    const auto end_timestamp = std::chrono::steady_clock::now();
    std::cout << "done" << std::endl;
    return 0;
}