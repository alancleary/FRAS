#ifndef INCLUDED_CFG_JAGGED_ARRAY_BP_INDEX
#define INCLUDED_CFG_JAGGED_ARRAY_BP_INDEX

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <malloc.h>
#include <new>
#include "cfg/jagged_array_bp.hpp"

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
class JaggedArrayBpIndex : public JaggedArrayBp
{
  private:

    const std::size_t intSizeBits = sizeof(int) * 8;

    // computes the max possible bit width of the index's values
    int packWidth(int index)
    {
      //return std::ceil(log2(index));
      return intSizeBits - __builtin_clz(index - 1);
    }

  public:

    JaggedArrayBpIndex(int numArrays): JaggedArrayBp(numArrays) { }

};

}

#endif
