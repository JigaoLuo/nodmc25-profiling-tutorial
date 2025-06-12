#include <iostream>
#include "workload/workload_set.h"
#include "btree_olc.h"
#include "coroutine/coroutine_round_robin_executor.h"
#include "system.h"
#include <perfcpp/sampler.h>
#include <perfcpp/hardware_info.h>
#include <perfcpp/analyzer/memory_access.h>
#include <sstream>
#include <fstream>
#include <chrono>
#include <filesystem>

int main() {
    auto tree = BTree<std::uint64_t, std::uint64_t>{};

    /// Create the workload.
    constexpr auto insert_requests = 50000000ULL;
    constexpr auto lookup_requests = 50000000ULL;
    auto benchmark_set = NumericWorkloadSet{insert_requests, lookup_requests};

    /// Create performance sampler.
    auto counter_definition = perf::CounterDefinition{};
    auto config = perf::SampleConfig{};

    auto sampler = perf::Sampler{counter_definition, config};
    if (perf::HardwareInfo::is_intel()) {
        sampler.trigger("mem-loads", perf::Precision::RequestZeroSkid, perf::Period{4000U});
        sampler.config().include_kernel(false);
    } else if (perf::HardwareInfo::is_amd()) {
        sampler.trigger("ibs_op_uops", perf::Precision::MustHaveZeroSkid, perf::Period{4000U});
    } else {
        std::cerr << "The underlying CPU is not supported." << std::endl;
        return 1;
    }
    sampler.values().logical_memory_address(true).latency(true).data_src(true);

    /// Execute the insert_requests phase.
    std::cout << "Executing " << insert_requests << " insert_requests requests..." << std::endl;
    CoroutineRoundRobinExecutor::execute(tree, benchmark_set.insert_requests());
    std::cout << "done" << std::endl;

    /// Execute the lookup phase.
    std::cout << "\nExecuting " << lookup_requests << " lookup requests..." << std::flush;
    sampler.start();
    const auto start_timestamp = std::chrono::steady_clock::now();
    CoroutineRoundRobinExecutor::execute(tree, benchmark_set.mixed_requests());
    const auto end_timestamp = std::chrono::steady_clock::now();
    sampler.stop();
    std::cout << "done" << std::endl;

    /// Analyze samples.
    auto memory_analyzer = perf::analyzer::MemoryAccess{};

    /// (1) Add node structures.
    auto [inner_node_structure, leaf_node_structure] = tree.get_node_structures();
    memory_analyzer.add(std::move(inner_node_structure));
    memory_analyzer.add(std::move(leaf_node_structure));

    /// (2) Add all addresses of the nodes to the memory analyzer.
    tree.traverse_tree_and_add_nodes(memory_analyzer);

    /// (3) Combine nodes and samples.
    auto result = memory_analyzer.map(sampler.result());
    std::cout << result.to_string() << std::endl;

    /// Process results and enrich data with metadata from the system and dump to file that will be uploaded via Sciebo.
    const auto lookup_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_timestamp - start_timestamp).count();
    const auto lookup_throughput = double(lookup_requests) / (double(lookup_ms) / 1000.);
    auto json_stream = std::stringstream{};
    json_stream
            << "{ \"metadata\":"
            << "{ \"cpu-model-name\": \"" << System::cpu_model_name() << "\", \"cpu-max-mhz\": "
            << System::cpu_max_mhz() << "}, "
            << "\"lookup-throughput\": " << lookup_throughput << ", \"results\": " << result.to_json() << "}"
            << std::flush;
    {
        std::filesystem::create_directory("tutorial-result");
        auto out_stream = std::ofstream{
                std::string{"tutorial-result/"}.append(System::create_identifier_from_cpu_model_and_hostname()).append(
                        ".json")};
        out_stream << json_stream.str() << std::flush;
    }

    return 0;
}
