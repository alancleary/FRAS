#ifndef INCLUDED_CFG_JAGGED_ARRAY_BP_MONO
#define INCLUDED_CFG_JAGGED_ARRAY_BP_MONO

#include <algorithm>
#include "cfg/jagged_array_bp.hpp"
#include <sdsl/bit_vectors.hpp>
#include <sdsl/util.hpp>

namespace cfg {

/**
 * Implements the JaggedArrayBp abstract class using a monotonic sequence of
 * packing sizes. Specifically, it computes the smallest bit width that a
 * subarray's values can be packed with and then packs them using that value or
 * the width of the preceeding subarray, whichever is larger. Assumes subarrays
 * are set in order.
 **/
class JaggedArrayBpMono : public JaggedArrayBp
{
  private:

    sdsl::sd_vector<> rulePackBitvector;
    sdsl::sd_vector<>::rank_1_type rulePackBitvectorRank;

    uint8_t* packSizes;  // no size will exceed 64

    int setPackWidth(int index, int* values, int length)
    {
      int width = 0;
      for (int i = 0; i < length; i++) {
        width = std::max(width, msb(values[i]));
      }
      if (index > 0) {
        width = std::max(width, (int) packSizes[index - 1]);
      }
      packSizes[index] = width;
      return width;
    }

    int getPackWidth(int index)
    {
      // index+1 because rank is exclusive [0, index) and we want inclusive [0, index]
      int r = rulePackBitvectorRank.rank(index + 1);
      return packSizes[r];
    }

    void indexPackSizes() {
      // create a bit vector for unique pack size positions
      sdsl::bit_vector tmpRulePackBitvector(numArrays, 0);
      //tmpRulePackBitvector[0] = 1;

      // compute the number of unique pack sizes and their locations
      int uniqueWidths = 1;
      for (int i = 1; i < numArrays; i++) {
        if (packSizes[i] != packSizes[i - 1]) {
          uniqueWidths += 1;
          tmpRulePackBitvector[i] = 1;
        }
      }

      // create the sparse bitvector structures
      rulePackBitvector = sdsl::sd_vector<>(tmpRulePackBitvector);
      rulePackBitvectorRank = sdsl::sd_vector<>::rank_1_type(&rulePackBitvector);
      sdsl::sd_vector<>::select_1_type rulePackBitvectorSelect(&rulePackBitvector);

      // create the unique pack size array
      uint8_t* uniquePackSizes = new uint8_t[uniqueWidths];
      uniquePackSizes[0] = packSizes[0];
      for (int i = 1; i < uniqueWidths; i++) {
        int j = rulePackBitvectorSelect.select(i);
        uniquePackSizes[i] = packSizes[j];
      }
      delete[] packSizes;
      packSizes = uniquePackSizes;
    }

  public:

    JaggedArrayBpMono(int numArrays): JaggedArrayBp(numArrays)
    {
      packSizes = new uint8_t[numArrays];
      for (int i = 0; i < numArrays; i++) {
        packSizes[i] = 0;
      }
    }

    ~JaggedArrayBpMono()
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
      int uniqueWidths = rulePackBitvectorRank.rank(rulePackBitvectorRank.size());
      memSize += uniqueWidths * sizeof(uint8_t);
      memSize += sdsl::size_in_bytes(rulePackBitvector);
      memSize += sdsl::size_in_bytes(rulePackBitvectorRank);
      return memSize;
    }

};

}

#endif
