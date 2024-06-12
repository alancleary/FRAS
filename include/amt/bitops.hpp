#ifndef INCLUDED_AMT_BITOPS
#define INCLUDED_AMT_BITOPS

#include <cstdint>

namespace amt {

/**
 * Returns a 64 bit uint with only the lowest bit set.
 *
 * @param value The uint to get the lowest bit from.
 * @return The lowest bit.
 */
uint64_t lowestOneBit(uint64_t value);

/**
 * Returns a 64 bit uint with only the highest bit set.
 *
 * @param value The uint to get the hight bit from.
 * @return The lowest bit.
 */
uint64_t highestOneBit(uint64_t value);

}

#endif
