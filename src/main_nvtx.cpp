#include <iostream>
#include "btree_olc.h"
#include "coroutine/coroutine_round_robin_executor.h"
#include <sstream>
#include <fstream>
#include <chrono>
#include <filesystem>

#include <nvtx3/nvtx3.hpp>

int main() {
    auto tree = BTree<std::uint64_t, std::uint64_t>{};

    /// Create the workload.
    constexpr auto insert_requests = 50000000ULL;
    constexpr auto lookup_requests = 50000000ULL;
    
    nvtxRangePushA("benchmark dataset"); // Begins NVTX range
    auto benchmark_set = NumericWorkloadSet{insert_requests, lookup_requests};
    nvtxRangePop(); // Ends NVTX range

    /// Execute the insert_requests phase.
    std::cout << "Executing " << insert_requests << " insert_requests requests..." << std::endl;
    {
        nvtx3::scoped_range r{"insert Time"};
        CoroutineRoundRobinExecutor::execute(tree, benchmark_set.insert_requests());
    }
    std::cout << "done" << std::endl;

    /// Execute the lookup phase.
    std::cout << "\nExecuting " << lookup_requests << " lookup requests..." << std::endl;
    const auto start_timestamp = std::chrono::steady_clock::now();
    {
        nvtx3::scoped_range r{"lookup Time"};
        CoroutineRoundRobinExecutor::execute(tree, benchmark_set.mixed_requests());
    }
    const auto end_timestamp = std::chrono::steady_clock::now();
    std::cout << "done" << std::endl;
    return 0;
}