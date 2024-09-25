#ifndef INCLUDED_CFG_JAGGED_ARRAY_BP_INDEX
#define INCLUDED_CFG_JAGGED_ARRAY_BP_INDEX

#include "cfg/jagged_array_bp.hpp"

namespace cfg {

/**
 * Implements the JaggedArrayBp abstract class using index-based bit packing.
 * Specifically, it is assumed that the values in every subarray will be less
 * than the subarray's index so the most significant bit of the index is used to
 * determine how many bits are used to pack each value in the subarray.
 **/
class JaggedArrayBpIndex : public JaggedArrayBp
{
  private:

    int setPackWidth(int index, int* values, int length)
    {
      return getPackWidth(index);
    }

    // computes the max possible bit width of the index's values
    int getPackWidth(int index)
    {
      return msb(index - 1);
    }

  public:

    JaggedArrayBpIndex(int numArrays): JaggedArrayBp(numArrays) { }

};

}

#endif
