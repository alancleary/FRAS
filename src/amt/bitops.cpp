#include "amt/bitops.hpp"

namespace amt {

uint64_t lowestOneBit(uint64_t value) {
    return value & - value;
}

uint64_t highestOneBit(uint64_t value) {
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return value ^ (value >> 1);
}
  
}
