#ifndef INCLUDED_CFG_RANDOM_ACCESS_AMT
#define INCLUDED_CFG_RANDOM_ACCESS_AMT

#include "amt/compressed_sum_set.hpp"
#include "cfg/random_access.hpp"

namespace cfg {

/** Indexes a CFG for random access using a tail-compressed array mapped trie with partial sums. */
template <class CFG_T>
class RandomAccessAMT : public RandomAccess<CFG_T>
{

private:

    static const int KEY_LENGTH = 6;

    amt::CompressedSumSet* cset;

    void setValues(amt::Set& set);

    uint64_t ruleSize(uint64_t* ruleSizes, int rule);

    void rankSelect(uint64_t i, int& rank, uint64_t& select);

public:

    static uint64_t getKey(uint8_t* key);
    static void setKey(uint8_t* key, uint64_t value);

    RandomAccessAMT(CFG_T* cfg);
    ~RandomAccessAMT();

};

}

#endif
