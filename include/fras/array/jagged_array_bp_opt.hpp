#ifndef INCLUDED_FRAS_ARRAY_JAGGED_ARRAY_BP_OPT
#define INCLUDED_FRAS_ARRAY_JAGGED_ARRAY_BP_OPT

#include <algorithm>
#include "fras/array/jagged_array_bp.hpp"
#include <sdsl/bit_vectors.hpp>
#include <sdsl/util.hpp>

namespace fras {

/**
 * Implements the JaggedArrayBp abstract class using the optimal packing size
 * for each subarray. Packing sizes themselves are stored in a packed array to
 * minimize space. Assumes the lsat subarray is set last.
 **/
class JaggedArrayBpOpt : public JaggedArrayBp
{
  private:

    uint8_t packSize;  // will not exceed 64
    uint8_t* packSizes;  // no size will exceed 64

    int setPackWidth(int index, int* values, int length)
    {
      int width = 0;
      for (int i = 0; i < length; i++) {
        width = std::max(width, msb(values[i]));
      }
      packSizes[index] = width;
      return width;
    }

    int getPackWidth(int index)
    {
      // unpack the pack size
      return unpackValue(packSizes, packSize, index);
    }

    void indexPackSizes() {
      // compute the number of unique pack sizes and their locations
      packSize = 0;
      int* unpackedSizes = new int[numArrays];
      for (int i = 0; i < numArrays; i++) {
        packSize = std::max(packSize, packSizes[i]);
        unpackedSizes[i] = (int) packSizes[i];
      }

      // created the packed packSize array
      int n = ((packSize * numArrays) + size - 1) / size;
      packSizes = (uint8_t*) realloc(packSizes, sizeof(uint8_t) * n);
      if (packSizes == NULL) {
        throw std::bad_alloc();
      }
      for (int i = 0; i < n; i++) {
        packSizes[i] = 0;
      }

      // pack the new array
      packArray(packSizes, packSize, unpackedSizes, numArrays);

      delete[] unpackedSizes;
    }

  public:

    JaggedArrayBpOpt(int numArrays): JaggedArrayBp(numArrays)
    {
      packSizes = new uint8_t[numArrays];
      for (int i = 0; i < numArrays; i++) {
        packSizes[i] = 0;
      }
    }

    ~JaggedArrayBpOpt()
    {
      delete[] packSizes;
    }

    void setArray(int index, int* values, int length)
    {
      JaggedArrayBp::setArray(index, values, length);
      // index the pack sizes if this is the last rule set
      if (index == numArrays - 1) {
        indexPackSizes();
      }
    }

    int getMemSize()
    {
      int memSize = JaggedArrayBp::getMemSize();
      int n = ((packSize * numArrays) + size - 1) / size;
      memSize += n * sizeof(uint8_t);
      return memSize;
    }

};

}

#endif
