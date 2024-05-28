#include <algorithm>
#include <fstream>
#include "amt/key.hpp"
#include "cfg/cfg.hpp"

#include <iostream>

namespace cfg {

// construction

CFG::CFG(): map(1024) { }

// construction from MR-Repair grammar

CFG* CFG::fromMrRepairFile(std::string filename)
{
    CFG* cfg = new CFG();

    std::ifstream reader(filename);
    std::string line;

    // read grammar specs
    std::getline(reader, line);
    cfg->textLength = std::stoi(line);
    std::getline(reader, line);
    cfg->numRules = std::stoi(line);
    std::getline(reader, line);
    cfg->startSize = std::stoi(line);
    cfg->rulesSize = 0;

    // prepare to read grammar
    //int* buff = new int[1024];
    int buffSize = cfg->numRules + CFG::MR_REPAIR_CHAR_SIZE;
    int rulesSize = buffSize + 1;  // +1 for start rule
    int** rules = new int*[rulesSize];
    rules[buffSize] = new int[cfg->startSize + 1];  // +1 for the dummy code
    int i, j, c, ruleLength;

    // read rules in order they were added to grammar, i.e. line-by-line
    for (i = CFG::MR_REPAIR_CHAR_SIZE; i < buffSize; i++) {
        for (j = 0; ;j++) {
            std::getline(reader, line);
            c = std::stoi(line);
            //buff[j] = c;
            // use start rule as a buffer
            rules[buffSize][j] = c;
            if (c == CFG::MR_REPAIR_DUMMY_CODE) {
                break;
            }
        }
        ruleLength = j;
        cfg->rulesSize += ruleLength;
        rules[i] = new int[ruleLength + 1];
        for (j = 0; j < ruleLength + 1; j++) {
            //rules[i][j] = buff[j];
            rules[i][j] = rules[buffSize][j];
        }
    }

    // read start rule
    for (i = 0; i < cfg->startSize; i++) {
        std::getline(reader, line);
        rules[buffSize][i] = std::stoi(line);
    }
    rules[buffSize][i] = CFG::MR_REPAIR_DUMMY_CODE;

    // convert start rule (and recursively all other rules) into a BBTrieMap
    CFG::fromMrRepairRules(cfg, rules, rulesSize);

    // clean up
    for (int i = 0; i < rulesSize; i++) {
        delete[] rules[i];
    }
    delete[] rules;

    return cfg;
}

void CFG::fromMrRepairRules(CFG* cfg, int** rules, int rulesSize)
{
    // initialize recursion variables
    int startRule = rulesSize - 1;
    int* ruleSizes = new int[rulesSize];
    int* ruleDepths = new int[rulesSize];
    int* references = new int[rulesSize];
    for (int i = 0; i < rulesSize; i++) {
        references[i] = -1;
    }
    uint8_t* key = new uint8_t[64];

    // build CFG recursively from start rule
    CFG::fromMrRepairRules(cfg, rules, startRule, ruleSizes, ruleDepths, references, key, 0);

    // save the depth
    cfg->depth = ruleDepths[startRule] - 1;

    // clean up
    delete[] ruleSizes;
    delete[] ruleDepths;
    delete[] references;
    delete[] key;
}

void CFG::fromMrRepairRules(CFG* cfg, int** rules, int ruleIdx, int* ruleSizes, int* ruleDepths , int* references, uint8_t* key, int seqPos)
{
    // add the rule to the map
    int* rule = rules[ruleIdx];
    int c;
    int len;
    for (int i = 0; rule[i] != CFG::MR_REPAIR_DUMMY_CODE; i++) {
        c = rule[i];
        len = amt::set6Int(key, seqPos);
        // this is the first occurrence of the (non-)terminal character
        if (references[c] == -1) {
            // the character is a terminal
            if (c < CFG::MR_REPAIR_CHAR_SIZE) {
                ruleSizes[c] = 1;
                ruleDepths[c] = 1;
                references[c] = c;  // MR_REPAIR_CHAR_SIZE == CHAR_SIZE
                cfg->map.set(key, len, references[c]);
            // the character is a non-terminal 
            } else {
                references[c] = CHAR_SIZE + seqPos;
                CFG::fromMrRepairRules(cfg, rules, c, ruleSizes, ruleDepths, references, key, seqPos);
            }
        } else {
            cfg->map.set(key, len, references[c]);
        }
        seqPos += ruleSizes[c];
        ruleSizes[ruleIdx] += ruleSizes[c];
        ruleDepths[ruleIdx] = std::max(ruleDepths[ruleIdx], ruleDepths[c] + 1);
    }
}

// random access

char CFG::get(uint32_t i)
{
    if (i < 0 || i >= textLength) {
        throw std::runtime_error("i is out of bounds");
    }

    uint8_t* key = new uint8_t[CFG::KEY_LENGTH];

    int len = amt::set6Int(key, i);
    uint64_t selected = map.predecessor(key, len);

    while (selected >= CFG::CHAR_SIZE) {
        i = (selected - CFG::CHAR_SIZE) + (i - amt::get6Int(key));
        len = amt::set6Int(key, i);
        selected = map.predecessor(key, len);
    }

    return (char) selected;
}

void CFG::get(std::ostream& out, uint32_t begin, uint32_t end)
{
    if (begin < 0 || end >= textLength || begin > end) {
        throw std::runtime_error("begin/end out of bounds");
    }

    uint8_t* key = new uint8_t[KEY_LENGTH * getDepth()];
    amt::set6Int(key, begin);
    uint64_t value = map.predecessor(key, KEY_LENGTH);
    uint32_t predecessor = amt::get6Int(key);
    //uint32_t ignore = begin - predecessor;

    //OutputStreamFilter filteredOut = new OutputStreamFilter(out, ignore);
    GetVisitorBasic visitor(*this, out);
    //GetVisitorCached visitor(*this, out);

    get(visitor, key, predecessor, end);
}

void CFG::get(GetVisitor& visitor, uint8_t* key, uint32_t begin, uint32_t end)
{
    map.visitRange(visitor, begin, end, CFG::KEY_LENGTH, key, visitor.keyPos);
    // process the interval between the last value visited and end
    visitor.processPrevious(key, end + 1);  // +1 to include the end
    visitor.previousKey = UINT32_MAX;
}

CFG::GetVisitor::GetVisitor(CFG& cfg, std::ostream& o): parent(cfg), out(o) { }

void CFG::GetVisitorBasic::processPrevious(uint8_t* key, uint32_t currentKey)
{
    // no rule to process
    if (previousKey == UINT32_MAX) {
        return;
    }

    // the rule is a terminal character
    if (previousValue < CHAR_SIZE) {
        out << (char) previousValue;
        return;
    }

    // recursively process the non-terminal
    uint32_t begin = previousValue - CHAR_SIZE;
    uint32_t end = begin + (currentKey - previousKey) - 1;
    previousKey = UINT32_MAX;
    keyPos += 6;
    parent.get(*this, key, begin, end);
    keyPos -= 6;
}

void CFG::GetVisitorBasic::visit(uint8_t* key, int len, uint64_t value)
{
    uint32_t currentKey = amt::get6Int(key, keyPos);
    processPrevious(key, currentKey);
    previousKey = currentKey;
    previousValue = value;
}

void CFG::GetVisitorCached::checkCache(uint32_t rule)
{
    // attempt to get the string
    auto stringItr = strings.find(rule);
    if (stringItr != strings.end()) {
        cachedRule = stringItr->second.first;
        cachedRuleLength = stringItr->second.second;
        cachedOffset = 0;
        return;
    }

    // attempt to get a pointer to another string
    auto pointerItr = pointers.find(rule);
    if (pointerItr == pointers.end()) {
        cachedRule = NULL;
        return;
    }

    // attempt to get a string for the pointer
    stringItr = strings.find(pointerItr->second.first);
    if (stringItr == strings.end()) {
        pointers.erase(rule);
        cachedRule = NULL;
        return;
    }

    // set the cached string
    cachedRule = stringItr->second.first;
    cachedRuleLength = stringItr->second.second;
    cachedOffset = pointerItr->second.second;
}

char* CFG::GetVisitorCached::cacheString(uint32_t rule, uint32_t length)
{
    char* chars = new char[length];
    strings[rule] = {chars, length};
    return chars;
}

void CFG::GetVisitorCached::cachePointer(uint32_t rule, uint32_t reference, uint32_t start)
{
    pointers[rule] = {reference, start};
}

void CFG::GetVisitorCached::cacheRule(uint32_t rule, uint32_t length)
{
    if (currentCache == NULL) {
        currentlyCaching = rule;
        currentCache = cacheString(rule, length);
        currentCachePosition = 0;
    } else {
        cachePointer(rule, currentlyCaching, currentCachePosition);
    }
}

void CFG::GetVisitorCached::writeChar(char c)
{
    out << c;
    if (currentCache != NULL) {
        currentCache[currentCachePosition++] = c;
    }
}

void CFG::GetVisitorCached::processPrevious(uint8_t* key, uint32_t currentKey)
{
    // no rule to process
    if (previousKey == UINT32_MAX) {
        return;
    }

    // the rule is a terminal character
    if (previousValue < CHAR_SIZE) {
        writeChar((char) previousValue);
        return;
    }

    // the rule is cached
    checkCache(previousValue);
    uint32_t length = currentKey - previousKey;
    if (cachedRule != NULL) {
        uint32_t i = 0;
        for (; i < std::min(length, cachedRuleLength - cachedOffset); i++) {
            writeChar(cachedRule[cachedOffset + i]);
        }
        if (i < length) {
            previousKey += i;
            previousValue += i;
            processPrevious(key, currentKey);
        }
    // create a new cache entry and recursively generate the string
    } else {
        uint32_t begin = previousValue - CHAR_SIZE;
        uint32_t end = begin + length - 1;

        cacheRule(previousValue, length);

        previousKey = UINT32_MAX;
        keyPos += CFG::KEY_LENGTH;
        parent.get(*this, key, begin, end);
        keyPos -= CFG::KEY_LENGTH;
    }

    // only non-terminals in the start rule are cached directly
    if (keyPos == 0) {
        currentCache = NULL;
    }
}

void CFG::GetVisitorCached::visit(uint8_t* key, int len, uint64_t value)
{
    uint32_t currentKey = amt::get6Int(key, keyPos);
    processPrevious(key, currentKey);
    previousKey = currentKey;
    previousValue = value;
}

}
