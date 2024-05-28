#include <bit>
#include "amt/key.hpp"

namespace amt {

int set6Int(uint8_t* key, uint32_t value, int pos /*= 0*/) {
    key[pos] = (uint8_t) ((value >> 30) & 0x3F);
    key[pos + 1] = (uint8_t) ((value >> 24) & 0x3F);
    key[pos + 2] = (uint8_t) ((value >> 18) & 0x3F);
    key[pos + 3] = (uint8_t) ((value >> 12) & 0x3F);
    key[pos + 4] = (uint8_t) ((value >> 6) & 0x3F);
    key[pos + 5] = (uint8_t) (value & 0x3F);
    return 6;
}

uint32_t get6Int(uint8_t* key, int pos /*= 0*/) {
    return ((key[pos] & 0x3F) << 30) |
           ((key[pos + 1] & 0x3F) << 24) |
           ((key[pos + 2] & 0x3F) << 18) |
           ((key[pos + 3] & 0x3F) << 12) |
           ((key[pos + 4] & 0x3F) << 6) |
           (key[pos + 5] & 0x3F);
}

uint8_t smallestKey(uint64_t value)
{
    return (uint8_t) std::countr_zero(value);
}

uint8_t largestKey(uint64_t value)
{
    return (uint8_t) (64 - std::countl_zero(value) - 1);
}

}
