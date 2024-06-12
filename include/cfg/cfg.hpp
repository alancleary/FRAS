#ifndef INCLUDED_CFG_CFG
#define INCLUDED_CFG_CFG

#include <string>
//#include "cfg/random_access.hpp"

namespace cfg {

/** Forward declare RandomAccess. */
//class RandomAccess;

/** Parses different grammar-compressed CFGs into a naive encoding. */
class CFG
{

//private:

// TODO: fix friend relationship and make these members private
public:
    static const int ALPHABET_SIZE = 256;

    static const int DUMMY_CODE = -1;  // UINT_MAX in MR-RePair C code

    uint64_t textLength = 0;
    int numRules = 0;
    int rulesSize = 0;
    int** rules;
    int startRule;
    int startSize = 0;
    int depth = 0;

private:
    void computeDepthAndTextSize(uint64_t* ruleSizes, int* ruleDepths, int rule);

    void reorderRules(uint64_t* ruleSizes);

    void postProcess();

public:

    uint64_t memSize()
    {
        return sizeof(int) * (startSize + rulesSize);
    }

    CFG();
    ~CFG();

    /**
     * Loads an MR-Repair grammar from a file.
     *
     * @param filename The file to load the grammar from.
     * @return The grammar that was loaded.
     * @throws Exception if the file cannot be read.
     */
    static CFG* fromMrRepairFile(std::string filename);

    /**
     * Loads a grammar from Navarro files.
     *
     * @param filenameC The grammar's C file.
     * @param filenameR The grammar's R file.
     * @return The grammar that was loaded.
     * @throws Exception if the files cannot be read.
     */
    static CFG* fromNavarroFiles(std::string filenameC, std::string filenameR);

    /**
     * Loads a grammar from Big-Repair files.
     *
     * @param filenameC The grammar's C file.
     * @param filenameR The grammar's R file.
     * @return The grammar that was loaded.
     * @throws Exception if the files cannot be read.
     */
    static CFG* fromBigRepairFiles(std::string filenameC, std::string filenameR);

    uint64_t getTextLength() const { return textLength; }
    int getNumRules() const { return numRules; }
    int getRulesSize() const { return rulesSize; }
    int getStartSize() const { return startSize; }
    int getTotalSize() const { return startSize + rulesSize; }
    int getDepth() const { return depth; }

    //friend class RandomAccess;
};

}

#endif
