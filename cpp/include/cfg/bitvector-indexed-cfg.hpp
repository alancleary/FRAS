#ifndef INCLUDED_CFG_BITVECTOR_INDEXED_CFG
#define INCLUDED_CFG_BITVECTOR_INDEXED_CFG

#include <cmath>
#include <ostream>
#include <string>
#include <utility>  // std::pair, std::make_pair
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support_v.hpp>
#include <sdsl/select_support_mcl.hpp>

namespace cfg {

/** A naive CFG representation indexed with an AMT Map. */
class BitvectorIndexedCFG
{

private:

    static const int KEY_LENGTH = 6;
    static const int ALPHABET_SIZE = 256;

    static const int DUMMY_CODE = -1;  // UINT_MAX in MR-RePair C code

    uint64_t textLength;
    int numRules;
    int startSize;
    int rulesSize;
    int depth;
    int** rules;
    int startRule;
    sdsl::bit_vector bitvector;
    // supports O(1) time rank queries on bitvector
    sdsl::rank_support_v5<> rank;
    // supports O(1) time select queries on bitvector
    //sdsl::select_support_mcl<> select;

public:

    //static uint64_t getKey(uint8_t* key);
    //static void setKey(uint8_t* key, uint64_t value);

    BitvectorIndexedCFG();
    ~BitvectorIndexedCFG();

    /**
     * Loads an MR-Repair grammar from a file.
     *
     * @param filename The file to load the grammar from.
     * @return The grammar that was loaded.
     * @throws Exception if the file cannot be read.
     */
    static BitvectorIndexedCFG* fromMrRepairFile(std::string filename);

    /**
     * Loads a grammar from Navarro files.
     *
     * @param filenameC The grammar's C file.
     * @param filenameR The grammar's R file.
     * @return The grammar that was loaded.
     * @throws Exception if the files cannot be read.
     */
    static BitvectorIndexedCFG* fromNavarroFiles(std::string filenameC, std::string filenameR);

    /**
     * Loads a grammar from Big-Repair files.
     *
     * @param filenameC The grammar's C file.
     * @param filenameR The grammar's R file.
     * @return The grammar that was loaded.
     * @throws Exception if the files cannot be read.
     */
    static BitvectorIndexedCFG* fromBigRepairFiles(std::string filenameC, std::string filenameR);

    uint64_t getTextLength() const { return textLength; }
    int getNumRules() const { return numRules; }
    int getStartSize() const { return startSize; }
    int getRulesSize() const { return rulesSize; }
    int getTotalSize() const { return startSize + rulesSize; }
    int getDepth() const { return depth; }

    unsigned int getBitvectorSize() const { return 64 * std::ceil(textLength / 64 + 1); }
    unsigned int getRankSize() const { return 0.25 * textLength; }
    unsigned int getRankSizeV5() const { return 0.0625 * textLength; }
    unsigned int getMemSize() const { return getBitvectorSize() + getRankSize(); }
    unsigned int getMemSizeV5() const { return getBitvectorSize() + getRankSizeV5(); }

    unsigned int getBitvectorSizeIl() const {
        unsigned int K = 512;
        return textLength * (1 + 64 / K);
    }
    unsigned int getRankSizeIl() const { return 128; }
    unsigned int getMemSizeIl() const { return getBitvectorSizeIl() + getRankSizeIl(); }

    //unsigned int choose(unsigned int n, unsigned int k) {
    //    if (k == 0) return 1;
    //    if (k > n / 2) return choose(n, n - k);
    //    return n * choose(n - 1, k - 1) / k;
    //}

    constexpr inline size_t binom(size_t n, size_t k) noexcept {
        return
          (        k> n  )? 0 :          // out of range
          (k==0 || k==n  )? 1 :          // edge
          (k==1 || k==n-1)? n :          // first
          (     k+k < n  )?              // recursive:
          (binom(n-1,k-1) * n)/k :       //  path to k=1   is faster
          (binom(n-1,k) * n)/(n-k);      //  path to k=n-1 is faster
    }

    unsigned int getBitvectorSizeRRR() {
        return std::ceil(std::log(binom(textLength, startSize)));
    }
    unsigned int getRankSizeRRR() { return 80; }
    unsigned int getMemSizeRRR() { return getBitvectorSizeRRR() + getRankSizeRRR(); }


    unsigned int getBitvectorSizeSparse() const {
        return startSize * (2 + std::log(textLength / startSize));
    }
    unsigned int getRankSizeSparse() const { return 64; }
    unsigned int getMemSizeSparse() const { return getBitvectorSizeSparse() + getRankSizeSparse(); }

    /**
      * Gets a substring in the original string.
      *
      * @param out The output stream to write the substring to.
      * @param begin The start position of the substring in the original string.
      * @param end The end position of the substring in the original string.
      * @throws Exception if begin or end is out of bounds.
      */
    void get(std::ostream& out, uint32_t begin, uint32_t end);

};

}

#endif
