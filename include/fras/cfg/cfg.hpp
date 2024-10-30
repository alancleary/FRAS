#ifndef INCLUDED_FRAS_CFG_CFG
#define INCLUDED_FRAS_CFG_CFG

#include <cstdint>
#include <string>

namespace fras {

/** Parses different grammar-compressed CFGs into a naive encoding. */
template <class JaggedArray_T>
class CFG
{

private:

    uint64_t textLength = 0;
    JaggedArray_T* rules;
    int numRules;
    int rulesSize = 0;
    int startRule;
    int startSize = 0;
    int depth = 0;

    void setRule(JaggedArray_T* rules, int rule, int* characters, int length)
    {
        rules->setArray(rule, characters, length);
    }
    void setRule(int rule, int* characters, int length)
    {
        setRule(this->rules, rule, characters, length);
    }

    void clearRule(JaggedArray_T* rules, int rule)
    {
        rules->clearArray(rule);
    }
    void setRule(int rule)
    {
        clearRule(this->rules, rule);
    }

    void computeDepthAndTextSize(uint64_t* ruleSizes, int* ruleDepths, int rule);
    void reorderRules(uint64_t* ruleSizes);
    void postProcess();

public:

    static const int ALPHABET_SIZE = 256;

    static const int DUMMY_CODE = 0;

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

    CFG(int numRules): numRules(numRules), startRule(numRules + CFG::ALPHABET_SIZE)
    {
        rules = new JaggedArray_T(numRules + CFG::ALPHABET_SIZE + 1);
    }
    ~CFG() { delete rules; };

    int get(std::size_t i, std::size_t j) { return rules->getValue(i, j); }

    const uint64_t& getTextLength() const { return textLength; }
    const int& getNumRules() const { return numRules; }
    const int& getRulesSize() const { return rulesSize; }
    const int& getStartRule() const { return startRule; }
    const int& getStartSize() const { return startSize; }
    const int getTotalSize() const { return startSize + rulesSize; }
    const int& getDepth() const { return depth; }

    int memSize() { return rules->getMemSize(); }

};

}

#endif
