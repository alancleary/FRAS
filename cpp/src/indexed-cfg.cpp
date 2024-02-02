#include <algorithm>
#include <fstream>
#include <stack>
#include "cfg-amt/amt/key.hpp"
#include "cfg-amt/indexed-cfg.hpp"

#include <iostream>

namespace cfg_amt {

// construction

IndexedCFG::IndexedCFG(): map(1024) { }

// destruction

IndexedCFG::~IndexedCFG() {
    for (int i = 0; i < rulesSize; i++) {
        delete[] rules[i];
    }
    delete[] rules;
}

// construction from MR-Repair grammar

IndexedCFG* IndexedCFG::fromMrRepairFile(std::string filename)
{
    IndexedCFG* cfg = new IndexedCFG();

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
    cfg->startRule = cfg->numRules + IndexedCFG::MR_REPAIR_CHAR_SIZE;
    int rulesSize = cfg->startRule + 1;  // +1 for start rule
    cfg->rules = new int*[rulesSize];
    int* ruleSizes = new int[rulesSize - 1];
    for (int i = 0; i < IndexedCFG::MR_REPAIR_CHAR_SIZE; i++) {
        ruleSizes[i] = 1;
    }
    cfg->rules[cfg->startRule] = new int[cfg->startSize + 1];  // +1 for the dummy code
    int i, j, c, ruleLength;

    // read rules in order they were added to grammar, i.e. line-by-line
    for (i = IndexedCFG::MR_REPAIR_CHAR_SIZE; i < cfg->startRule; i++) {
        for (j = 0; ;j++) {
            std::getline(reader, line);
            c = std::stoi(line);
            // use start rule as a buffer
            cfg->rules[cfg->startRule][j] = c;
            if (c == IndexedCFG::MR_REPAIR_DUMMY_CODE) {
                break;
            }
            ruleSizes[i] += ruleSizes[c];
        }
        ruleLength = j;
        cfg->rulesSize += ruleLength;
        cfg->rules[i] = new int[ruleLength + 1];
        for (j = 0; j < ruleLength + 1; j++) {
            cfg->rules[i][j] = cfg->rules[cfg->startRule][j];
        }
    }

    // read start rule
    uint32_t pos = 0;
    uint8_t* key = new uint8_t[IndexedCFG::KEY_LENGTH];
    int len;
    for (i = 0; i < cfg->startSize; i++) {
        // get the (non-)terminal character
        std::getline(reader, line);
        c = std::stoi(line);
        cfg->rules[cfg->startRule][i] = c;
        // encode a pointer to its index in the AMT Map
        len = set6Int(key, pos);
        cfg->map.set(key, len, i);
        pos += ruleSizes[c];
    }
    cfg->rules[cfg->startRule][i] = IndexedCFG::MR_REPAIR_DUMMY_CODE;

    // clean up
    delete[] ruleSizes;

    return cfg;
}

// random access

void IndexedCFG::get(std::ostream& out, uint32_t begin, uint32_t end)
{
    if (begin < 0 || end >= textLength || begin > end) {
        throw std::runtime_error("begin/end out of bounds");
    }
    uint32_t length = end - begin;

    uint8_t* key = new uint8_t[KEY_LENGTH];
    set6Int(key, begin);
    int i = (int) map.predecessor(key, KEY_LENGTH);
    //uint32_t predecessor = get6Int(key);
    //uint32_t ignore = begin - predecessor;

    // TODO: stacks should be preallocated to size of max depth
    int r = startRule;
    std::stack<int> ruleStack;
    std::stack<int> indexStack;
    for (uint32_t j = 0; j < length;) {
        // end of rule
        if (rules[r][i] == MR_REPAIR_DUMMY_CODE) {
            r = ruleStack.top();
            ruleStack.pop();
            i = indexStack.top();
            indexStack.pop();
        // terminal character 
        } else if (rules[r][i] < MR_REPAIR_CHAR_SIZE) {
            out << (char) rules[r][i];
            i++;
            j++;
        // non-terminal character
        } else {
            ruleStack.push(r);
            r = rules[r][i];
            indexStack.push(i + 1);
            i = 0;
        }
    }
}

}
