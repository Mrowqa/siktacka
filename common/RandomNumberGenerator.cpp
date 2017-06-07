#include <common/RandomNumberGenerator.hpp>

#include <ctime>


RandomNumberGenerator::RandomNumberGenerator() noexcept
        : next_val(time(nullptr)) {
}


void RandomNumberGenerator::set_seed(uint64_t seed) noexcept {
    next_val = seed;
}


uint64_t RandomNumberGenerator::next() noexcept {
    auto result = next_val;
    next_val = (next_val * 279470273) % 4294967291;
    return result;
}


uint64_t RandomNumberGenerator::peek() noexcept {
    return next_val;
}
