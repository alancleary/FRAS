#ifndef INCLUDED_CFG_AMT_BITVECTOR_INDEXED_CFG
#define INCLUDED_CFG_AMT_BITVECTOR_INDEXED_CFG

#include <cmath>
#include <ostream>
#include <string>
#include <utility>  // std::pair, std::make_pair
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support_v.hpp>
#include <sdsl/select_support_mcl.hpp>

namespace cfg_amt {

/** A naive CFG representation indexed with an AMT Map. */
class BitvectorIndexedCFG
{

private:

    static const int KEY_LENGTH = 6;
    static const int ALPHABET_SIZE = 256;

    static const int DUMMY_CODE = -1;  // UINT_MAX in MR-RePair C code

    unsigned int textLength;
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

    unsigned int getTextLength() const { return textLength; }
    int getNumRules() const { return numRules; }
    int getStartSize() const { return startSize; }
    int getRulesSize() const { return rulesSize; }
    int getTotalSize() const { return startSize + rulesSize; }
    int getDepth() const { return depth; }
    unsigned int getBitvectorSize() const { return 64 * std::ceil(textLength / 64 + 1); }
    unsigned int getRankSize() const { return 0.25 * textLength; }
    unsigned int getMemSize() const { return getBitvectorSize() + getRankSize(); }

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
