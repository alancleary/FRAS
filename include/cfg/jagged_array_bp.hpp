#ifndef INCLUDED_CFG_JAGGED_ARRAY_BP
#define INCLUDED_CFG_JAGGED_ARRAY_BP

#include "cfg/jagged_array.hpp"

namespace cfg {

/**
 * An abstract class that provides an interface for jagged array implementations based on bit packing.
 * The class also provides a commmon byte representation with code for encoding/decoding code.
 **/
class JaggedArrayBp : public JaggedArray
{
  protected:

    const std::size_t size = sizeof(uint8_t) * 8;
    uint8_t** arrays;

  public:

    JaggedArrayBp(int numArrays): JaggedArray(numArrays) {
      // initialize the jagged array
      arrays = new uint8_t*[numArrays];
      for (int i = 0; i < numArrays; i++) {
        arrays[i] = NULL;
      }
    }

    ~JaggedArrayBp()
    {
      for (int i = 0; i < numArrays; i++) {
        free(arrays[i]);
      }
      delete[] arrays;
    }

    virtual int packWidth(int index) = 0;

    int getMemSize() {
      int memSize = 0;

      // get the index that the character starts at and its offset
      for (int i = 0; i < numArrays; i++) {
        uint8_t* array = arrays[i];
        if (array == NULL) continue;
        int j = 0;
        int offset = 0;
        // get the number of bits each value will be packed in
        int width = packWidth(i);
        // mask the bits left of the packed value
        int value = (1 << width) - 1;
        while (value != 0) {
          // the value is completely stored in entry j
          if (offset + width <= size) {
            // get the packed value
            value &= array[j] >> size - width - offset;
            offset += width;
            if (offset >= size) {
              offset %= size;
              j += 1;
            }
          // the value is split between two or more entries, starting at j
          } else {
            int shift = width + offset - size;
            value &= array[j++] << shift;
            shift -= size;
            while (shift > 0) {
              value |= array[j++] << shift;
              shift -= size;
            }
            if (shift == 0) {
              value |= array[j++];
              offset = 0;
            // shift < 0
            } else {
              value |= array[j++] >> -shift;
              offset = size + shift;
            }
          }
        }
        memSize += j;
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

      return value;
    }

};

}

#endif
