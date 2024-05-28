#ifndef INCLUDED_CFG_AMT_CFG
#define INCLUDED_CFG_AMT_CFG

#include <ostream>
#include <string>
#include <unordered_map>
#include <utility>
#include "cfg-amt/amt/map.hpp"

#include <unordered_set>

namespace cfg_amt {

/** A representation of a context-free grammar built on the MAT Map. */
class CFG
{

private:

    class GetVisitor : public MapVisitor
    {
    protected:
        CFG& parent;
        std::ostream& out;
    public:
        int keyPos = 0;
        uint32_t previousKey = UINT32_MAX;
        uint64_t previousValue;

        GetVisitor(CFG& cfg, std::ostream& o);

        virtual void processPrevious(uint8_t* key, uint32_t currentKey) = 0;
        virtual void visit(uint8_t* key, int len, uint64_t value) = 0;
    };

    class GetVisitorBasic : public GetVisitor
    {
    public:
        using GetVisitor::GetVisitor;

        void processPrevious(uint8_t* key, uint32_t currentKey);
        void visit(uint8_t* key, int len, uint64_t value);
    };

    class GetVisitorCached : public GetVisitor
    {
    private:
        uint32_t currentlyCaching;
        char* currentCache = NULL;
        uint32_t currentCachePosition;

        char* cachedRule = NULL;
        uint32_t cachedRuleLength;
        uint32_t cachedOffset;

        // maps rules to the strings they generate
        std::unordered_map<uint32_t, std::pair<char*, uint32_t>> strings;
        // maps nested rules into the string of a rule they're nested in
        // value always has length 3: [0] = rule, [1] = start
        std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> pointers;

        void checkCache(uint32_t rule);
        char* cacheString(uint32_t rule, uint32_t length);
        void cachePointer(uint32_t rule, uint32_t reference, uint32_t start);
        void cacheRule(uint32_t rule, uint32_t length);

        void writeChar(char c);
    public:
        using GetVisitor::GetVisitor;

        void processPrevious(uint8_t* key, uint32_t currentKey);
        void visit(uint8_t* key, int len, uint64_t value);
    };

    static const int KEY_LENGTH = 6;
    static const int CHAR_SIZE = 256;

    static const int MR_REPAIR_CHAR_SIZE = 256;
    static const int MR_REPAIR_DUMMY_CODE = -1;  // UINT_MAX in MR-RePair C code

    int textLength;
    int numRules;
    int startSize;
    int rulesSize;
    int depth;
    Map map;

    //static int printMrRepairRules(int** rules, int rule, int pos);
    static void fromMrRepairRules(CFG* cfg, int** rules, int rulesSize);
    static void fromMrRepairRules(CFG* cfg, int** rules, int ruleIdx, int* ruleSizes, int* ruleDepths , int* references, uint8_t* key, int seqPos);

    void get(GetVisitor& visitor, uint8_t* key, uint32_t begin, uint32_t end);

public:

    CFG();

    /**
     * Loads an MR-Repair grammar from a file.
     *
     * @param filename The file to load the grammar from.
     * @return The grammar that was loaded.
     * @throws Exception if the file cannot be read.
     */
    static CFG* fromMrRepairFile(std::string filename);

    int getTextLength() { return textLength; }
    int getNumRules() { return numRules; }
    int getStartSize() { return startSize; }
    int getRulesSize() { return rulesSize; }
    int getTotalSize() { return startSize + rulesSize; }
    int getDepth() { return depth; }
    uint64_t getNumMapEntries() { return map.size(); }

    /**
      * Gets the character at position i in the original string.
      *
      * @param i The position in the original string.
      * @return The decoded character.
      * @throws Exception if i is out of bounds.
      */
    char get(uint32_t i);

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
