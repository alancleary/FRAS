#include <algorithm>
#include <cstdio>  // FILE
#include <fstream>
#include <sys/stat.h>
#include "cfg/cfg.hpp"

namespace cfg {

// construction

CFG::CFG() { }

// destruction

CFG::~CFG()
{
    for (int i = CFG::ALPHABET_SIZE; i < startRule + 1; i++) {
        delete[] rules[i];
    }
    delete[] rules;
}

// private

void CFG::computeDepthAndTextSize()
{
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
    computeDepthAndTextSize(ruleSizes, ruleDepths, startRule);
    textLength = ruleSizes[startRule];
    depth = ruleDepths[startRule];
    delete[] ruleSizes;
    delete[] ruleDepths;
}

void CFG::computeDepthAndTextSize(uint64_t* ruleSizes, int* ruleDepths, int rule)
{
    if (ruleSizes[rule] != 0) return;

    int c;
    for (int i = 0; (c = rules[rule][i]) != CFG::DUMMY_CODE; i++) {
        if (ruleSizes[c] == 0) {
            computeDepthAndTextSize(ruleSizes, ruleDepths, c);
        }
        ruleSizes[rule] += ruleSizes[c];
        ruleDepths[rule] = std::max(ruleDepths[rule], ruleDepths[c]);
    }
    ruleDepths[rule]++;
}

// load grammars

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

    // prepare to read grammar
    cfg->startRule = cfg->numRules + CFG::ALPHABET_SIZE;
    cfg->rules = new int*[cfg->startRule + 1];  // +1 for start rule
    cfg->rules[cfg->startRule] = new int[cfg->startSize + 1];  // +1 for the dummy code
    int j, c, ruleLength;

    // read rules in the order they were added to grammar, i.e. line-by-line
    for (int i = CFG::ALPHABET_SIZE; i < cfg->startRule; i++) {
        for (j = 0; ;j++) {
            std::getline(reader, line);
            c = std::stoi(line);
            // use start rule as a buffer
            cfg->rules[cfg->startRule][j] = c;
            if (c == CFG::DUMMY_CODE) {
                break;
            }
        }
        ruleLength = j;
        cfg->rulesSize += ruleLength;
        cfg->rules[i] = new int[ruleLength + 1];
        for (j = 0; j <= ruleLength; j++) {
            cfg->rules[i][j] = cfg->rules[cfg->startRule][j];
        }
    }

    // read start rule
    for (int i = 0; i < cfg->startSize; i++) {
        // get the (non-)terminal character
        std::getline(reader, line);
        c = std::stoi(line);
        cfg->rules[cfg->startRule][i] = c;
    }
    cfg->rules[cfg->startRule][cfg->startSize] = CFG::DUMMY_CODE;

    // compute grammar depth and text length
    cfg->computeDepthAndTextSize();

    return cfg;
}

// construction from Navarro grammar

CFG* CFG::fromNavarroFiles(std::string filenameC, std::string filenameR)
{
    typedef struct { int left, right; } Tpair;

    CFG* cfg = new CFG();

    // get the .R file size
    struct stat s;
    stat(filenameR.c_str(), &s);
    int len = s.st_size;

    // open the .R file
    FILE* rFile = fopen(filenameR.c_str(), "r");

    // read the alphabet size
    int alphabetSize;
    fread(&alphabetSize, sizeof(int), 1, rFile);
    cfg->numRules = (len - sizeof(int) - alphabetSize) / sizeof(Tpair);
    cfg->rulesSize = cfg->numRules * 2;  // each rule is a pair
    cfg->startRule = cfg->numRules + CFG::ALPHABET_SIZE;

    // read the alphabet map, i.e. \Sigma -> [0..255]
    char map[256];
    fread(&map, sizeof(char), alphabetSize, rFile);

    // prepare to read grammar
    cfg->rules = new int*[cfg->startRule + 1];  // +1 for start rule

    // read the rule pairs
    Tpair p;
    int c;
    for (int i = CFG::ALPHABET_SIZE; i < cfg->startRule; i++) {
        fread(&p, sizeof(Tpair), 1, rFile);
        cfg->rules[i] = new int[3];  // +1 for the dummy code
        if (p.left < alphabetSize) {
            c = (unsigned char) map[p.left];
        } else {
            c = p.left - alphabetSize + CFG::ALPHABET_SIZE;
        }
        cfg->rules[i][0] = c;
        if (p.right < alphabetSize) {
            c = (unsigned char) map[p.right];
        } else {
            c = p.right - alphabetSize + CFG::ALPHABET_SIZE;
        }
        cfg->rules[i][1] = c;
        cfg->rules[i][2] = CFG::DUMMY_CODE;
    }

    // close the .R file
    fclose(rFile);

    // get the .C file size
    stat(filenameC.c_str(), &s);
    cfg->startSize = s.st_size / sizeof(int);
    cfg->rules[cfg->startRule] = new int[cfg->startSize + 1];  // +1 for the dummy code

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
        cfg->rules[cfg->startRule][i] = c;
    }
    cfg->rules[cfg->startRule][cfg->startSize] = CFG::DUMMY_CODE;

    // compute grammar depth and text length
    cfg->computeDepthAndTextSize();

    return cfg;
}

// construction from BigRePair grammar

CFG* CFG::fromBigRepairFiles(std::string filenameC, std::string filenameR)
{
    typedef struct { unsigned int left, right; } Tpair;

    CFG* cfg = new CFG();

    // get the .R file size
    struct stat s;
    stat(filenameR.c_str(), &s);
    int len = s.st_size;

    // open the .R file
    FILE* rFile = fopen(filenameR.c_str(), "r");

    // read the alphabet size
    int alphabetSize;
    fread(&alphabetSize, sizeof(int), 1, rFile);  // NOTE: alphabetSize is always 256
    cfg->numRules = (len - sizeof(int)) / sizeof(Tpair);
    cfg->rulesSize = cfg->numRules * 2;  // each rule is a pair
    cfg->startRule = cfg->numRules + CFG::ALPHABET_SIZE;

    // prepare to read grammar
    cfg->rules = new int*[cfg->startRule + 1];  // +1 for start rule

    // read the rule pairs
    Tpair p;
    int c;
    for (int i = CFG::ALPHABET_SIZE; i < cfg->startRule; i++) {
        fread(&p, sizeof(Tpair), 1, rFile);
        cfg->rules[i] = new int[3];  // +1 for the dummy code
        if (p.left < alphabetSize) {
            c = (unsigned char) p.left;
        } else {
            c = p.left;  // already offset by alphabetSize
        }
        cfg->rules[i][0] = c;
        if (p.right < alphabetSize) {
            c = (unsigned char) p.right;
        } else {
            c = p.right;  // already offset by alphabetSize
        }
        cfg->rules[i][1] = c;
        cfg->rules[i][2] = CFG::DUMMY_CODE;
    }

    // close the .R file
    fclose(rFile);

    // get the .C file size
    stat(filenameC.c_str(), &s);
    cfg->startSize = s.st_size / sizeof(unsigned int);
    cfg->rules[cfg->startRule] = new int[cfg->startSize + 1];  // +1 for the dummy code

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
        cfg->rules[cfg->startRule][i] = c;
    }
    cfg->rules[cfg->startRule][cfg->startSize] = CFG::DUMMY_CODE;

    // compute grammar depth and text length
    cfg->computeDepthAndTextSize();

    return cfg;
}

}
