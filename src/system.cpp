#include "system.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <unistd.h>

std::uint32_t System::cpu_max_mhz() {
    auto cpu_info_max_freq = std::ifstream{"/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"};
    std::string line;
    std::smatch match;

    if (cpu_info_max_freq.is_open()) {
        if (std::getline(cpu_info_max_freq, line)) {
            auto stringstream = std::stringstream{};
            std::uint64_t frequency_in_khz;
            stringstream << line;
            stringstream >> frequency_in_khz;

            return frequency_in_khz / 1000ULL;
        }
    }

    return 0;
}

std::string System::cpu_model_name() {
    auto cpu_info = std::ifstream{"/proc/cpuinfo"};
    std::string line;
    auto model_pattern = std::regex{"model name\\s+:\\s+(.*)"};
    std::smatch match;

    if (cpu_info.is_open()) {
        while (std::getline(cpu_info, line)) {
            if (std::regex_search(line, match, model_pattern) && match.size() > 1U) {
                return match[1].str();
            }
        }
    }

    return "unknown";
}

std::string System::create_identifier_from_cpu_model_and_hostname() {
    std::string name;
    for (char ch: System::cpu_model_name()) {
        if (std::isalnum(ch)) {
            name += ch;
        } else if (std::isspace(ch)) {
            name += '_';
        }
    }

    // Get hostname
    char hostname[1024];
    ::gethostname(hostname, sizeof(hostname));

    // Create a 4-digit hash from the hostname
    const auto hostname_hash = std::hash<std::string>()(std::string{hostname}) * 653491ULL % 10000ULL;
    name += "_" + std::to_string(hostname_hash);

    return name;
}