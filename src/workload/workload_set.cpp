#include "workload_set.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <numeric>

NumericWorkloadSet::NumericWorkloadSet(const std::string &insert_workload_file,
                                       const std::string &mixed_workload_file) {
    auto parse = [](auto &file_stream, std::vector<NumericTuple> &data_set) {
        std::srand(1337);
        std::string op_name;
        std::uint64_t key{};

        while (file_stream >> op_name >> key) {
            if (op_name == "INSERT") {
                data_set.emplace_back(NumericTuple{NumericTuple::Type::INSERT, key, std::rand()});
            } else if (op_name == "READ") {
                data_set.emplace_back(NumericTuple{NumericTuple::Type::LOOKUP, key});
            } else if (op_name == "UPDATE") {
                data_set.emplace_back(NumericTuple{NumericTuple::Type::UPDATE, key, std::rand()});
            }
        }
    };

    std::mutex out_mutex;
    auto fill_thread = std::thread{[this, &out_mutex, &parse, &insert_workload_file]() {
        auto workload_file = std::ifstream{insert_workload_file};
        if (workload_file.good()) {
            parse(workload_file, this->_data_sets[static_cast<std::size_t>(phase::INSERT)]);
        } else {
            std::lock_guard<std::mutex> lock{out_mutex};
            std::cerr << "Could not open workload file '" << insert_workload_file << "'." << std::endl;
        }
    }};

    auto mixed_thread = std::thread{[this, &out_mutex, &parse, &mixed_workload_file]() {
        auto workload_file = std::ifstream{mixed_workload_file};
        if (workload_file.good()) {
            parse(workload_file, this->_data_sets[static_cast<std::size_t>(phase::MIXED)]);
        } else {
            std::lock_guard<std::mutex> lock{out_mutex};
            std::cerr << "Could not open workload file '" << mixed_workload_file << "'." << std::endl;
        }
    }};

    fill_thread.join();
    mixed_thread.join();
}

NumericWorkloadSet::NumericWorkloadSet(const std::uint64_t count_insert, const std::uint64_t count_lookup) {
    auto generate = [](const NumericTuple::Type type, const std::uint64_t max, std::vector<NumericTuple> &data_set) {
        std::srand(std::uintptr_t(data_set.data()));

        /// Fill data.
        data_set.reserve(max);
        for (auto i = 0ULL; i < max; ++i) {
            data_set.emplace_back(type, i, i);
        }

        /// Shuffle.
        std::random_device random_device;
        auto random_engine = std::default_random_engine{random_device()};
        std::shuffle(data_set.begin(), data_set.end(), random_engine);
    };


    auto fill_thread = std::thread{[this, &generate, count_insert]() {
        generate(NumericTuple::Type::INSERT, count_insert, this->_data_sets[static_cast<std::size_t>(phase::INSERT)]);
    }};

    auto mixed_thread = std::thread{[this, &generate, count_lookup]() {
        generate(NumericTuple::Type::LOOKUP, count_lookup, this->_data_sets[static_cast<std::size_t>(phase::MIXED)]);
    }};

    fill_thread.join();
    mixed_thread.join();
}

namespace benchmark {
    std::ostream &operator<<(std::ostream &stream, const NumericWorkloadSet &workload) {

        stream << "insert_requests phase: " << workload[phase::INSERT].size() << " inserts" << " / "
               << "mixed_requests phase: " << workload[phase::MIXED].size() << " requests";

        return stream << std::flush;
    }
} // namespace benchmark
