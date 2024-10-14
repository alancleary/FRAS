#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <sys/stat.h>
#include "cfg/cfg.hpp"
#include "cfg/jagged_array_bp_index.hpp"
#include "cfg/jagged_array_bp_mono.hpp"
#include "cfg/jagged_array_bp_opt.hpp"
#include "cfg/jagged_array_int.hpp"

namespace cfg {

// private

template <class JaggedArray_T>
void CFG<JaggedArray_T>::computeDepthAndTextSize(uint64_t* ruleSizes, int* ruleDepths, int rule)
{
    if (ruleSizes[rule] != 0) return;

    int c;
    for (int i = 0; (c = get(rule, i)) != CFG::DUMMY_CODE; i++) {
        if (ruleSizes[c] == 0) {
            computeDepthAndTextSize(ruleSizes, ruleDepths, c);
        }
        ruleSizes[rule] += ruleSizes[c];
        ruleDepths[rule] = std::max(ruleDepths[rule], ruleDepths[c]);
    }
    ruleDepths[rule]++;
}

template <class JaggedArray_T>
void CFG<JaggedArray_T>::reorderRules(uint64_t* ruleSizes)
{
    // count how many times each expansion length occurs
    std::map<uint64_t, int> sizeMap;
    uint64_t size;
    for (int i = CFG::ALPHABET_SIZE; i < startRule; i++) {
        size = ruleSizes[i];
        if (!sizeMap.contains(size)) {
            sizeMap[size] = 0;
        }
        sizeMap[size] += 1;
    }

    // compute the last occurrence of each expansion length in the new ordering
    int offset = CFG::ALPHABET_SIZE - 1;
    int nextOffset = offset;
    for (auto& [size, count] : sizeMap) {
        nextOffset += count;
        count += offset;
        offset = nextOffset;
    }

    // assign new rule characters using the last occurrences
    int* newOrdering = new int[startRule + 1];
    int* newOrderingReversed = new int[startRule + 1];
    for (int i = CFG::ALPHABET_SIZE; i < startRule; i++) {
        size = ruleSizes[i];
        newOrdering[i] = sizeMap[size]--;
        newOrderingReversed[newOrdering[i]] = i;
    }
    newOrdering[startRule] = startRule;
    newOrderingReversed[startRule] = startRule;

    // reorder the rules and update characters
    // NOTE: assigning rules in order is required by some jagged arrays
    JaggedArray_T* newRules = new JaggedArray_T(startRule + 1);
    int* ruleBuffer = new int[startSize + 1];  // +1 for the dummy code
    int c, newIndex;
    for (int i = CFG::ALPHABET_SIZE; i <= startRule; i++) {
        int oldIndex = newOrderingReversed[i];
        int j = 0;
        do {
            c = this->get(oldIndex, j);
            if (c < CFG::ALPHABET_SIZE) {
                ruleBuffer[j] = c;
            } else {
                ruleBuffer[j] = newOrdering[c];
            }
            j += 1;
        } while (c != CFG::DUMMY_CODE);
        setRule(newRules, i, ruleBuffer, j);
        clearRule(rules, oldIndex);
    }
    delete rules;
    rules = newRules;

    // clean up
    delete[] ruleBuffer;
    delete[] newOrdering;
    delete[] newOrderingReversed;
}

template <class JaggedArray_T>
void CFG<JaggedArray_T>::postProcess()
{
    // prepare post-processing structures
    uint64_t* ruleSizes = new uint64_t[startRule + 1];
    int* ruleDepths = new int[startRule + 1];
    for (int i = 0; i < CFG::ALPHABET_SIZE; i++) {
        ruleSizes[i] = 1;
        ruleDepths[i] = 1;
    }
    for (int i = CFG::ALPHABET_SIZE; i <= startRule; i++) {
        ruleSizes[i] = 0;
        ruleDepths[i] = 0;
    }

    // compute the depth and text length
    computeDepthAndTextSize(ruleSizes, ruleDepths, startRule);
    textLength = ruleSizes[startRule];
    depth = ruleDepths[startRule];

    // clean up depths
    delete[] ruleDepths;

    // order the rules by expansion length, shortest to longest
    reorderRules(ruleSizes);

    // clean up rule sizes
    delete[] ruleSizes;
}

// load grammars

template <class JaggedArray_T>
CFG<JaggedArray_T>* CFG<JaggedArray_T>::fromMrRepairFile(std::string filename)
{
    std::ifstream reader(filename);
    std::string line;

    // read grammar specs
    std::getline(reader, line);
    int textLength = std::stoi(line);
    std::getline(reader, line);
    int numRules = std::stoi(line);
    std::getline(reader, line);
    int startSize = std::stoi(line);

    // prepare to read grammar
    CFG* cfg = new CFG<JaggedArray_T>(numRules);
    cfg->textLength = std::stoi(line);
    cfg->startSize = std::stoi(line);

    // assume the start rule is the largest to create a rule buffer
    int* ruleBuffer = new int[cfg->startSize + 1];  // +1 for the dummy code
    int j, c, ruleLength;

    // read rules in the order they were added to grammar, i.e. line-by-line
    for (int i = CFG::ALPHABET_SIZE; i < cfg->startRule; i++) {
        int j = 0;
        std::getline(reader, line);
        c = std::stoi(line);
        while ((c = std::stoi(line)) != -1) {
            ruleBuffer[j++] = c;
            std::getline(reader, line);
        }
        ruleBuffer[j++] = CFG::DUMMY_CODE;
        cfg->rulesSize += j;
        cfg->setRule(i, ruleBuffer, j);
    }

    // read start rule
    for (int i = 0; i < cfg->startSize; i++) {
        // get the (non-)terminal character
        std::getline(reader, line);
        c = std::stoi(line);
        ruleBuffer[i] = c;
    }
    ruleBuffer[cfg->startSize] = CFG::DUMMY_CODE;
    cfg->setRule(cfg->startRule, ruleBuffer, cfg->startSize + 1);

    // compute grammar depth and text length
    cfg->postProcess();

    // cleanup
    delete[] ruleBuffer;

    return cfg;
}

// construction from Navarro grammar

template <class JaggedArray_T>
CFG<JaggedArray_T>* CFG<JaggedArray_T>::fromNavarroFiles(std::string filenameC, std::string filenameR)
{
    typedef struct { int left, right; } Tpair;

    // get the .R file size
    struct stat s;
    stat(filenameR.c_str(), &s);
    int len = s.st_size;

    // open the .R file
    FILE* rFile = fopen(filenameR.c_str(), "r");

    // read the alphabet size
    int alphabetSize;
    fread(&alphabetSize, sizeof(int), 1, rFile);
    int numRules = (len - sizeof(int) - alphabetSize) / sizeof(Tpair);
    CFG* cfg = new CFG<JaggedArray_T>(numRules);
    cfg->rulesSize = numRules * 2;  // each rule is a pair

    // read the alphabet map, i.e. \Sigma -> [0..255]
    char map[256];
    fread(&map, sizeof(char), alphabetSize, rFile);

    // prepare to read grammar
    int* ruleBuffer = new int[3];  // +1 for the dummy code

    // read the rule pairs
    Tpair p;
    int c;
    for (int i = CFG::ALPHABET_SIZE; i < cfg->startRule; i++) {
        fread(&p, sizeof(Tpair), 1, rFile);
        if (p.left < alphabetSize) {
            c = (unsigned char) map[p.left];
        } else {
            c = p.left - alphabetSize + CFG::ALPHABET_SIZE;
        }
        ruleBuffer[0] = c;
        if (p.right < alphabetSize) {
            c = (unsigned char) map[p.right];
        } else {
            c = p.right - alphabetSize + CFG::ALPHABET_SIZE;
        }
        ruleBuffer[1] = c;
        ruleBuffer[2] = CFG::DUMMY_CODE;
        cfg->setRule(i, ruleBuffer, 3);
    }

    // close the .R file
    fclose(rFile);

    // get the .C file size
    stat(filenameC.c_str(), &s);
    cfg->startSize = s.st_size / sizeof(int);
    // resize the buffer
    delete[] ruleBuffer;
    ruleBuffer = new int[cfg->startSize + 1];  // +1 for the dummy code

    // open the .C file
    FILE* cFile = fopen(filenameC.c_str(), "r");

    // read the start rule
    int t;
    for (int i = 0; i < cfg->startSize; i++) {
        fread(&t, sizeof(int), 1, cFile);
        if (t < alphabetSize) {
            c = (unsigned char) map[t];
        } else {
            c = t - alphabetSize + CFG::ALPHABET_SIZE;
        }
        ruleBuffer[i] = c;
    }
    ruleBuffer[cfg->startSize] = CFG::DUMMY_CODE;
    cfg->setRule(cfg->startRule, ruleBuffer, cfg->startSize + 1);

    // compute grammar depth and text length
    cfg->postProcess();

    // cleanup
    delete[] ruleBuffer;

    return cfg;
}

// construction from BigRePair grammar

template <class JaggedArray_T>
CFG<JaggedArray_T>* CFG<JaggedArray_T>::fromBigRepairFiles(std::string filenameC, std::string filenameR)
{
    typedef struct { unsigned int left, right; } Tpair;

    // get the .R file size
    struct stat s;
    stat(filenameR.c_str(), &s);
    unsigned int len = s.st_size;

    // open the .R file
    FILE* rFile = fopen(filenameR.c_str(), "r");

    // read the alphabet size
    int alphabetSize;
    fread(&alphabetSize, sizeof(int), 1, rFile);  // NOTE: alphabetSize is always 256
    int numRules = (len - sizeof(int)) / sizeof(Tpair);
    CFG* cfg = new CFG<JaggedArray_T>(numRules);
    cfg->rulesSize = cfg->numRules * 2;  // each rule is a pair
    cfg->startRule = cfg->numRules + CFG::ALPHABET_SIZE;

    // prepare to read grammar
    int* ruleBuffer = new int[cfg->startRule + 1];  // +1 for start rule

    // read the rule pairs
    Tpair p;
    int c;
    for (int i = CFG::ALPHABET_SIZE; i < cfg->startRule; i++) {
        fread(&p, sizeof(Tpair), 1, rFile);
        if (p.left < alphabetSize) {
            c = (unsigned char) p.left;
        } else {
            c = p.left;  // already offset by alphabetSize
        }
        ruleBuffer[0] = c;
        if (p.right < alphabetSize) {
            c = (unsigned char) p.right;
        } else {
            c = p.right;  // already offset by alphabetSize
        }
        ruleBuffer[1] = c;
        ruleBuffer[2] = CFG::DUMMY_CODE;
        cfg->setRule(i, ruleBuffer, 3);
    }

    // close the .R file
    fclose(rFile);

    // get the .C file size
    stat(filenameC.c_str(), &s);
    cfg->startSize = s.st_size / sizeof(unsigned int);

    // open the .C file
    FILE* cFile = fopen(filenameC.c_str(), "r");

    // read the start rule
    uint64_t pos = 0;
    int t;
    for (int i = 0; i < cfg->startSize; i++) {
        fread(&t, sizeof(unsigned int), 1, cFile);
        if (t < alphabetSize) {
            c = (unsigned char) t;
        } else {
            c = t;  // already offset by alphabetSize
        }
        ruleBuffer[i] = c;
    }
    ruleBuffer[cfg->startSize] = CFG::DUMMY_CODE;
    cfg->setRule(cfg->startRule, ruleBuffer, cfg->startSize + 1);

    // compute grammar depth and text length
    cfg->postProcess();

    // cleanup
    delete[] ruleBuffer;

    return cfg;
}

// instantiate the class
template class CFG<JaggedArrayBpIndex>;
template class CFG<JaggedArrayBpMono>;
template class CFG<JaggedArrayBpOpt>;
template class CFG<JaggedArrayInt>;

}
