#pragma once

#include <cstdint>
#include <string>

class System {
public:
    [[nodiscard]] static std::string cpu_model_name();

    [[nodiscard]] static std::uint32_t cpu_max_mhz();

    [[nodiscard]] static std::string create_identifier_from_cpu_model_and_hostname();
};