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
    static const int CHAR_SIZE = 256;

    static const int MR_REPAIR_CHAR_SIZE = 256;
    static const int MR_REPAIR_DUMMY_CODE = -1;  // UINT_MAX in MR-RePair C code

    int textLength;
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

    int getTextLength() { return textLength; }
    int getNumRules() { return numRules; }
    int getStartSize() { return startSize; }
    int getRulesSize() { return rulesSize; }
    int getTotalSize() { return startSize + rulesSize; }
    //int getDepth() { return depth; }
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
