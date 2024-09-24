#ifndef INCLUDED_CFG_JAGGED_ARRAY_BP_INDEX
#define INCLUDED_CFG_JAGGED_ARRAY_BP_INDEX

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <malloc.h>
#include <new>
#include "cfg/jagged_array.hpp"

//#include <cassert>
#include <iostream>

namespace cfg {

/**
 * Implements the jagged array abstract class using bit packing.
 * Specifically, it is assumed that the values in every subarray
 * will be less than the subarray's index so the most significant
 * bit of the index is used to determine how many bits are used
 * to pack each value in the subarray.
 **/
class JaggedArrayBpIndex : public JaggedArray
{
  private:

    const std::size_t size = sizeof(uint8_t) * 8;
    const std::size_t intSizeBits = sizeof(int) * 8;
    uint8_t** arrays;

    // computes the max possible bit width of the index's values
    int packWidth(int index)
    {
      //return std::ceil(log2(index));
      return intSizeBits - __builtin_clz(index - 1);
    }

  public:

    JaggedArrayBpIndex(int numArrays): JaggedArray(numArrays)
    {
      // initialize the jagged array
      arrays = new uint8_t*[numArrays];
      for (int i = 0; i < numArrays; i++) {
        arrays[i] = NULL;
      }
    };

    ~JaggedArrayBpIndex()
    {
      for (int i = 0; i < numArrays; i++) {
        free(arrays[i]);
      }
      delete[] arrays;
    }

    int getMemSize() {
      int memSize = 0;
      for (int i = 0; i < numArrays; i++) {
        uint8_t* array = arrays[i];
        if (array == NULL) continue;
        int j = 0;
        int zeros = 0;
        while (zeros < 4) {
          uint8_t value = array[j++];
          if (value == 0) zeros++;
          else zeros = 0;
        }
        memSize += j - 4;
      }
      return memSize;
    }

    void setArray(int index, int* values, int length)
    {

      // get the number of bits each value will be packed in
      int width = packWidth(index);

      // compute the smallest uint8_t array that will hold all the bits
      int n = ((width * length) + size - 1) / size;

      // allocate and initialize the new array
      //memSize -= malloc_usable_size(arrays[index]);
      uint8_t* array = arrays[index] = (uint8_t*) realloc(arrays[index], sizeof(uint8_t) * n);
      if (array == NULL) {
        throw std::bad_alloc();
      }
      //memSize += malloc_usable_size(array);
      for (int i = 0; i < n; i++) {
        array[i] = 0;
      }

      // pack the values into the array
      int j = 0;
      int offset = 0;
      for (int i = 0; i < length; i++) {
        int value = values[i];
        // get the index that the character starts at and its offset
        //assert(j == (i * width) / size);
        //assert(offset == (i * width) % size);
        // the value is completely stored in entry j
        if (offset + width <= size) {
          array[j] |= value << size - width - offset;
          offset += width;
          if (offset >= size) {
            offset %= size;
            j += 1;
          }
        // the value is split between two or more entries, starting at j
        } else {
          int shift = width + offset - size;
          do {
            array[j++] |= value >> shift;
            shift -= size;
          } while(shift > 0);
          if (shift == 0) {
            array[j++] |= value;
            offset = 0;
          // shift < 0
          } else {
            array[j] |= value << -shift;
            offset = size + shift;
          }
        }
      }
      
    }

    void clearArray(int index)
    {
      free(arrays[index]);
      arrays[index] = NULL;
    }

    int getValue(int index, int i)
    {
      uint8_t* array = arrays[index];

      // get the number of bits each value will be packed in
      int width = packWidth(index);

      //for (int i = 0; i < length; i++) {
      // mask the bits left of the packed value
      int value = (1 << width) - 1;
      // get the index that the character starts at and its offset
      int j = (i * width) / size;
      int offset = (i * width) % size;
      // the value is completely stored in entry j
      if (offset + width <= size) {
        // get the packed value
        value &= array[j] >> size - width - offset;
      // the value is split between two or more entries, starting at j
      } else {
        int shift = width + offset - size;
        //do {
        //  value &= array[j++] << shift;
        //  shift -= size;
        //} while(shift > 0);
        value &= array[j++] << shift;
        shift -= size;
        while (shift > 0) {
          value |= array[j++] << shift;
          shift -= size;
        }
        if (shift == 0) {
          value |= array[j++];
        // shift < 0
        } else {
          value |= array[j++] >> -shift;
        }
      }
      //}
      return value;
    }
};

}

#endif
