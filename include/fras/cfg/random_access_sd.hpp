#ifndef INCLUDED_FRAS_CFG_RANDOM_ACCESS_SD
#define INCLUDED_FRAS_CFG_RANDOM_ACCESS_SD

#include "fras/cfg/random_access.hpp"
#include "fras/array/jagged_array_bp_index.hpp"
#include "fras/array/jagged_array_bp_mono.hpp"
#include "fras/array/jagged_array_int.hpp"
#include <sdsl/bit_vectors.hpp>
#include <sdsl/util.hpp>

namespace fras {

/**
 * Indexes a CFG for random access using a bit vector.
 * NOTE: this class requires that the CFG rules are in smallest-expansion-first order.
 **/
template <class CFG_T>
class RandomAccessSD : public RandomAccess<CFG_T>
{

private:

    sdsl::sd_vector<> startBitvector;
    sdsl::sd_vector<>::rank_1_type startBitvectorRank;
    sdsl::sd_vector<>::select_1_type startBitvectorSelect;

    sdsl::sd_vector<> expansionBitvector;
    sdsl::sd_vector<>::rank_1_type expansionBitvectorRank;

    uint64_t* expansionSizes;

    void initializeBitvectors()
    {
        sdsl::bit_vector tmpStartBitvector(this->cfg->getTextLength(), 0);
        // startRule = numRules + CFG::ALPHABET_SIZE
        sdsl::bit_vector tmpExpansionBitvector(this->cfg->getStartRule(), 0);

        // prepare to compute rule sizes
        uint64_t* ruleSizes = new uint64_t[this->cfg->getStartRule()];
        for (int i = 0; i < CFG_T::ALPHABET_SIZE; i++) {
            ruleSizes[i] = 1;
        }
        for (int i = CFG_T::ALPHABET_SIZE; i < this->cfg->getStartRule(); i++) {
            ruleSizes[i] = 0;
        }

        // set the start bitvector
        uint64_t pos = 0;
        int c;
        for (int i = 0; i < this->cfg->getStartSize(); i++) {
            c = this->cfg->get(this->cfg->getStartRule(), i);
            tmpStartBitvector[pos] = 1;
            pos += ruleSize(ruleSizes, c);
        }
        startBitvector = sdsl::sd_vector<>(tmpStartBitvector);

        // set the expansion bitvector and count the number of unique expansions
        uint64_t previousSize = 1;
        int numExpansions = 1;  // 1 will be in the array but not have a bit set
        for (int i = 0; i < this->cfg->getStartRule(); i++) {
            if (ruleSizes[i] > previousSize) {
                numExpansions++;
                previousSize = ruleSizes[i];
                tmpExpansionBitvector[i] = 1;
            }
        }
        expansionBitvector = sdsl::sd_vector<>(tmpExpansionBitvector);

        // initialize the expansion array
        expansionSizes = new uint64_t[numExpansions];
        previousSize = 1;
        numExpansions = 0;
        expansionSizes[numExpansions++] = previousSize;
        for (int i = 0; i < this->cfg->getStartRule(); i++) {
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
        for (int i = 0; (c = this->cfg->get(rule, i)) != CFG_T::DUMMY_CODE; i++) {
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

    uint64_t memSize()
    {
        //uint64_t numExpansions = sdsl::util::cnt_one_bits(expansionBitvector);
        //uint64_t expansionSize = sizeof(uint64_t) * numExpansions;
        //return expansionSize + sizeof(startBitvector) + sizeof(expansionBitvector);
        uint64_t numExpansions = expansionBitvectorRank.rank(expansionBitvector.size());
        uint64_t expansionSize = sizeof(uint64_t) * numExpansions;

        uint64_t startBitvectorSize = sdsl::size_in_bytes(startBitvector);
        uint64_t startBitvectorRankSize = sdsl::size_in_bytes(startBitvectorRank);
        uint64_t startBitvectorSelectSize = sdsl::size_in_bytes(startBitvectorSelect);

        uint64_t expansionBitvectorSize = sdsl::size_in_bytes(expansionBitvector);
        uint64_t expansionBitvectorRankSize = sdsl::size_in_bytes(expansionBitvectorRank);

        return startBitvectorSelectSize + startBitvectorRankSize + startBitvectorSelectSize +
               expansionBitvectorSize + expansionBitvectorRankSize +
               expansionSize;
    }

    RandomAccessSD(CFG_T* cfg): RandomAccess<CFG_T>(cfg)
    {
        initializeBitvectors();
        startBitvectorRank = sdsl::sd_vector<>::rank_1_type(&startBitvector);
        startBitvectorSelect = sdsl::sd_vector<>::select_1_type(&startBitvector);
        expansionBitvectorRank = sdsl::sd_vector<>::rank_1_type(&expansionBitvector);
    }

    ~RandomAccessSD()
    {
        delete[] expansionSizes;
    };

};

// instantiate the class
template class RandomAccessSD<CFG<JaggedArrayBpIndex>>;
template class RandomAccessSD<CFG<JaggedArrayBpMono>>;
template class RandomAccessSD<CFG<JaggedArrayInt>>;

}

#endif
