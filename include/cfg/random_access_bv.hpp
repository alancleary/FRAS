#ifndef INCLUDED_CFG_RANDOM_ACCESS_BV
#define INCLUDED_CFG_RANDOM_ACCESS_BV

#include "cfg/random_access.hpp"

namespace cfg {

/** Indexes a CFG for random access using a bit vector. */
template <class sdsl_bv, class sdsl_rank, class sdsl_select>
class RandomAccessBV : public RandomAccess
{

private:

    sdsl_bv bitvector;
    sdsl_rank bitvector_rank;
    sdsl_select bitvector_select;

    void setBits()
    {
        uint64_t* ruleSizes = new uint64_t[cfg->startRule];
        for (int i = 0; i < CFG::ALPHABET_SIZE; i++) {
            ruleSizes[i] = 1;
        }
        for (int i = CFG::ALPHABET_SIZE; i < cfg->startRule; i++) {
            ruleSizes[i] = 0;
        }
        uint64_t pos = 0;
        int c;
        for (int i = 0; i < cfg->startSize; i++) {
            c = cfg->get(cfg->startRule, i);
            bitvector[pos] = 1;
            pos += ruleSize(ruleSizes, c);
        }
        delete[] ruleSizes;
    }

    uint64_t ruleSize(uint64_t* ruleSizes, int rule)
    {
        if (ruleSizes[rule] != 0) return ruleSizes[rule];

        int c;
        for (int i = 0; (c = cfg->get(rule, i)) != CFG::DUMMY_CODE; i++) {
            if (ruleSizes[c] == 0) {
                ruleSize(ruleSizes, c);
            }
            ruleSizes[rule] += ruleSizes[c];
        }

        return ruleSizes[rule];
    }

    void rankSelect(uint64_t i, int& rank, uint64_t& select)
    {
        // i+1 because rank is exclusive [0, i) and we want inclusive [0, i]
        rank = bitvector_rank.rank(i + 1);
        select = bitvector_select.select(rank);
    }

public:

    RandomAccessBV(CFG* cfg): RandomAccess(cfg)
    {
        bitvector = sdsl_bv(cfg->textLength, 0);
        setBits();
        bitvector_rank = sdsl_rank(&bitvector);
        bitvector_select = sdsl_select(&bitvector);
    }

};

}

#endif
