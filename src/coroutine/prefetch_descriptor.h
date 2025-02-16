#pragma once

#include <cstdint>
#include <limits>

class PrefetchDescriptor {
public:
    PrefetchDescriptor() noexcept = default;

    ~PrefetchDescriptor() noexcept = default;

    [[nodiscard]] static PrefetchDescriptor make_read(void *address) noexcept {
        return PrefetchDescriptor{(std::numeric_limits<std::uintptr_t>::max() >> 1) & std::uintptr_t(address)};
    }

    [[nodiscard]] static PrefetchDescriptor make_write(void *address) noexcept {
        return PrefetchDescriptor{
                (1ULL << 63) & ((std::numeric_limits<std::uintptr_t>::max() >> 1) & std::uintptr_t(address))};
    }

    [[nodiscard]] bool is_write() const noexcept {
        return _data >> 63;
    }

    [[nodiscard]] void *address() const noexcept {
        return reinterpret_cast<void *>(_data & (std::numeric_limits<std::uintptr_t>::max() >> 1));
    }

    [[nodiscard]] bool empty() const noexcept { return _data == 0U; }

private:
    explicit PrefetchDescriptor(const std::uint64_t data) : _data(data) {}

    /// Prefetch descriptor with
    ///     MSB:    1 = Write, 0 = Read
    ///     Rest:   Data address to prefetch
    std::uintptr_t _data{0U};
};