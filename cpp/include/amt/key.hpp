#ifndef INCLUDED_AMT_KEY
#define INCLUDED_AMT_KEY

#include <cstdint>

namespace amt {

/**
 * Converts a 32 bit int to a 6 byte key.
 *
 * @param key The array to store the key in.
 * @param value The int to convert into a key.
 * @param pos Optional offset into key.
 * @return The number of bytes in the key (always 6).
 */
int set6Int(uint8_t* key, uint32_t value, int pos = 0);

/**
 * Converts a 6 byte key to its 32 bit int value.
 *
 * @param key The key to convert.
 * @param pos Optional offset into key.
 * @return The int value of the key.
 */
uint32_t get6Int(uint8_t* key, int pos = 0);

/**
 * Gets the smallest key byte stored in the given 64 bit value.
 *
 * @param value The 64 bit value.
 * @return The smallest key byte.
 */
uint8_t smallestKey(uint64_t value);

/**
 * Gets the largest key byte stored in the given 64 bit value.
 *
 * @param value The 64 bit value.
 * @return The largest key byte.
 */
uint8_t largestKey(uint64_t value);

}

#endif
