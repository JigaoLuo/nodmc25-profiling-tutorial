#pragma once

#include "phase.h"
#include <array>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

class NumericTuple {
public:
    enum class Type : std::uint8_t {
        INSERT,
        LOOKUP,
        UPDATE,
        DELETE
    };

    constexpr NumericTuple(const Type type, const std::uint64_t key) : _type(type), _key(key) {}

    constexpr NumericTuple(const Type type, const std::uint64_t key, const std::int64_t value)
            : _type(type), _key(key), _value(value) {
    }

    NumericTuple(NumericTuple &&) noexcept = default;

    NumericTuple(const NumericTuple &) = default;

    ~NumericTuple() = default;

    NumericTuple &operator=(NumericTuple &&) noexcept = default;

    NumericTuple &operator=(const NumericTuple &) noexcept = default;

    [[nodiscard]] std::uint64_t key() const { return _key; };

    [[nodiscard]] std::int64_t value() const { return _value; }

    bool operator==(const Type type) const { return _type == type; }

private:
    Type _type;
    std::uint64_t _key;
    std::int64_t _value = 0;
};

class NumericWorkloadSet {
    friend std::ostream &operator<<(std::ostream &stream, const NumericWorkloadSet &workload_set);

public:
    NumericWorkloadSet() = default;

    NumericWorkloadSet(std::uint64_t count_insert, std::uint64_t count_lookup);

    NumericWorkloadSet(const std::string &insert_workload_file, const std::string &mixed_workload_file);

    NumericWorkloadSet(NumericWorkloadSet &&) noexcept = default;

    ~NumericWorkloadSet() = default;

    NumericWorkloadSet &operator=(NumericWorkloadSet &&) noexcept = default;


    [[nodiscard]] const std::vector<NumericTuple> &insert_requests() const noexcept { return _data_sets[0]; }

    [[nodiscard]] const std::vector<NumericTuple> &mixed_requests() const noexcept { return _data_sets[1]; }

    const std::vector<NumericTuple> &operator[](const phase phase) const noexcept {
        return _data_sets[static_cast<std::uint16_t>(phase)];
    }

    explicit operator bool() const { return insert_requests().empty() == false || mixed_requests().empty() == false; }

private:
    std::array<std::vector<NumericTuple>, 2> _data_sets;
};