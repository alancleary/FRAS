#ifndef INCLUDED_CFG_RANDOM_ACCESS_BV
#define INCLUDED_CFG_RANDOM_ACCESS_BV

#include "cfg/random_access.hpp"

namespace cfg {

/**
 * Indexes a CFG for random access using a bit vector.
 * NOTE: this class requires that the CFG rules are in smallest-expansion-first order.
 **/
template <class CFG_T, class sdsl_bv, class sdsl_rank, class sdsl_select>
class RandomAccessBV : public RandomAccess
{

private:

    sdsl_bv startBitvector;
    sdsl_rank startBitvectorRank;
    sdsl_select startBitvectorSelect;

    sdsl_bv expansionBitvector;
    sdsl_rank expansionBitvectorRank;

    uint64_t* expansionSizes;

    void setBits()
    {
        // prepare to compute rule sizes
        uint64_t* ruleSizes = new uint64_t[cfg->startRule];
        for (int i = 0; i < CFG_T::ALPHABET_SIZE; i++) {
            ruleSizes[i] = 1;
        }
        for (int i = CFG_T::ALPHABET_SIZE; i < cfg->startRule; i++) {
            ruleSizes[i] = 0;
        }

        // set the start bitvector
        uint64_t pos = 0;
        int c;
        for (int i = 0; i < cfg->startSize; i++) {
            c = cfg->get(cfg->startRule, i);
            startBitvector[pos] = 1;
            pos += ruleSize(ruleSizes, c);
        }

        // set the expansion bitvector and count the number of unique expansions
        uint64_t previousSize = 1;
        int numExpansions = 1;  // 1 will be in the array but not have a bit set
        for (int i = 0; i < cfg->startRule; i++) {
            if (ruleSizes[i] > previousSize) {
                numExpansions++;
                previousSize = ruleSizes[i];
                expansionBitvector[i] = 1;
            }
        }

        // initialize the expansion array
        expansionSizes = new uint64_t[numExpansions];
        previousSize = 1;
        numExpansions = 0;
        expansionSizes[numExpansions++] = previousSize;
        for (int i = 0; i < cfg->startRule; i++) {
            if (ruleSizes[i] > previousSize) {
                previousSize = ruleSizes[i];
                expansionSizes[numExpansions++] = previousSize;
            }
        }

        // clean up
        delete[] ruleSizes;
    }

    uint64_t ruleSize(uint64_t* ruleSizes, int rule)
    {
        if (ruleSizes[rule] != 0) return ruleSizes[rule];

        int c;
        for (int i = 0; (c = cfg->get(rule, i)) != CFG_T::DUMMY_CODE; i++) {
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
        rank = startBitvectorRank.rank(i + 1);
        select = startBitvectorSelect.select(rank);
    }

    uint64_t expansionSize(int rule)
    {
        // i+1 because rank is exclusive [0, i) and we want inclusive [0, i]
        int rank = expansionBitvectorRank.rank(rule + 1);
        return expansionSizes[rank];
    }

public:

    RandomAccessBV(CFG_T* cfg): RandomAccess(cfg)
    {
        startBitvector = sdsl_bv(cfg->textLength, 0);
        // startRule = numRules + CFG::ALPHABET_SIZE
        expansionBitvector = sdsl_bv(cfg->startRule, 0);
        setBits();
        startBitvectorRank = sdsl_rank(&startBitvector);
        startBitvectorSelect = sdsl_select(&startBitvector);
        expansionBitvectorRank = sdsl_rank(&expansionBitvector);
    }

    ~RandomAccessBV()
    {
        delete[] expansionSizes;
    };

};

}

#endif
