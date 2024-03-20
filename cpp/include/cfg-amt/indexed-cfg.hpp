#ifndef INCLUDED_CFG_AMT_INDEXED_CFG
#define INCLUDED_CFG_AMT_INDEXED_CFG

#include <ostream>
#include <string>
#include "cfg-amt/amt/map.hpp"

namespace cfg_amt {

/** A naive CFG representation indexed with an AMT Map. */
class IndexedCFG
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
    Map map;

public:

    IndexedCFG();
    ~IndexedCFG();

    /**
     * Loads an MR-Repair grammar from a file.
     *
     * @param filename The file to load the grammar from.
     * @return The grammar that was loaded.
     * @throws Exception if the file cannot be read.
     */
    static IndexedCFG* fromMrRepairFile(std::string filename);

    /**
     * Loads a grammar from Navarro files.
     *
     * @param filenameC The grammar's C file.
     * @param filenameR The grammar's R file.
     * @return The grammar that was loaded.
     * @throws Exception if the files cannot be read.
     */
    static IndexedCFG* fromNavarroFiles(std::string filenameC, std::string filenameR);

    /**
     * Loads a grammar from Big-Repair files.
     *
     * @param filenameC The grammar's C file.
     * @param filenameR The grammar's R file.
     * @return The grammar that was loaded.
     * @throws Exception if the files cannot be read.
     */
    static IndexedCFG* fromBigRepairFiles(std::string filenameC, std::string filenameR);

    unsigned int getTextLength() const { return textLength; }
    int getNumRules() const { return numRules; }
    int getStartSize() const { return startSize; }
    int getRulesSize() const { return rulesSize; }
    int getTotalSize() const { return startSize + rulesSize; }
    int getDepth() const { return depth; }
    uint64_t getNumMapEntries() { return map.size(); }

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
