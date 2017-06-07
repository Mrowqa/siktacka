#pragma once

#include <cstdint>


class RandomNumberGenerator {
private:
    uint64_t next_val;

public:
    RandomNumberGenerator() noexcept;

    void set_seed(uint64_t seed) noexcept;
    uint64_t next() noexcept;
    uint64_t peek() noexcept;
};


