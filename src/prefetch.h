#pragma once

#include <cstdint>

class SWPrefetcher {
public:
    enum class Target : std::uint8_t {
        ALL = 3U,
        L2 = 2U,
        L3 = 1U,
        NTA = 0U
    };

    template<std::uint32_t F, std::uint32_t C, Target T = Target::ALL>
    inline static void prefetch(std::int64_t *data) {
        constexpr auto items_per_cacheline = 64U / sizeof(std::int64_t);
        for (auto i = F * items_per_cacheline; i < (C + F) * items_per_cacheline; i += items_per_cacheline) {
            __builtin_prefetch(&data[i], 0, static_cast<std::uint8_t>(T));
        }
    }

    template<std::uint32_t F, std::uint32_t C, Target T = Target::ALL>
    inline static void prefetch(void *data) {
        prefetch<F, C, T>(reinterpret_cast<std::int64_t *>(data));
    }

    inline static void prefetchw(void *const data) {
        __builtin_prefetch(data, 1, 0);
    }
};
