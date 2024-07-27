#include <algorithm>
#include <cstdio>  // FILE
#include <fstream>
#include <map>
#include <sys/stat.h>
#include <cmath>
#include <vector>
#include <variant>
#include <bitset>
#include <iostream>
#include "cfg/cfg.hpp"

// need ways to store inner array lengths and rule lengths somehow, probably better solutions that are smaller
int* cfg::CFG::innerLengths;
int* cfg::CFG::ruleLengths;
void** cfg::CFG::newRules;

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

//this unpacks the bitpacked character based on rule and position among right hand side rules
int CFG::unpack(int rule, int pos) {
    // Get the MSB used for every RHS rule
    int divider = MSB(rule - 1);
    // Get the smallest array size for that rule
    int bitSize = typeSize(divider);
    // actual position after bitwidths taken into account
    int realPos = divider * pos;
    // have to store lengths of the packed bits (6 characters packed into 4 elements, this variable gets the 6);
    int innerLength = innerLengths[rule];


    // bit manipulation to isolate correct bits
    if (bitSize == 8) {
        uint8_t* subarray = static_cast<uint8_t*>(newRules[rule]);
        int totalBits = innerLength * 8;
        if (realPos + divider > totalBits) {
            throw std::out_of_range("realPos exceeds bitStore size");
        }
        int byteIndex = realPos / 8;
        int bitOffset = realPos % 8;
        uint64_t value = 0;
        for (int i = 0; i < divider; i++) {
            value |= ((subarray[byteIndex] >> (7 - bitOffset)) & 1) << (divider - 1 - i);
            bitOffset++;
            if (bitOffset == 8) {
                bitOffset = 0;
                byteIndex++;
            }
        }
        return static_cast<int>(value);
    } else if (bitSize == 16) {
        uint16_t* subarray = static_cast<uint16_t*>(newRules[rule]);
        int totalBits = innerLength * 16;
        if (realPos + divider > totalBits) {
            throw std::out_of_range("realPos exceeds bitStore size");
        }
        int byteIndex = realPos / 16;
        int bitOffset = realPos % 16;
        uint64_t value = 0;
        for (int i = 0; i < divider; i++) {
            value |= ((subarray[byteIndex] >> (15 - bitOffset)) & 1) << (divider - 1 - i);
            bitOffset++;
            if (bitOffset == 16) {
                bitOffset = 0;
                byteIndex++;
            }
        }
        return static_cast<int>(value);
    } else if (bitSize == 32) {
        uint32_t* subarray = static_cast<uint32_t*>(newRules[rule]);
        int totalBits = innerLength * 32;
        if (realPos + divider > totalBits) {
            throw std::out_of_range("realPos exceeds bitStore size");
        }
        int byteIndex = realPos / 32;
        int bitOffset = realPos % 32;
        uint64_t value = 0;
        for (int i = 0; i < divider; i++) {
            value |= ((subarray[byteIndex] >> (31 - bitOffset)) & 1) << (divider - 1 - i);
            bitOffset++;
            if (bitOffset == 32) {
                bitOffset = 0;
                byteIndex++;
            }
        }
        return static_cast<int>(value);
    } else {
        uint64_t* subarray = static_cast<uint64_t*>(newRules[rule]);
        int totalBits = innerLength * 64;
        if (realPos + divider > totalBits) {
            throw std::out_of_range("realPos exceeds bitStore size");
        }
        int byteIndex = realPos / 64;
        int bitOffset = realPos % 64;
        uint64_t value = 0;
        for (int i = 0; i < divider; i++) {
            value |= ((subarray[byteIndex] >> (63 - bitOffset)) & 1) << (divider - 1 - i);
            bitOffset++;
            if (bitOffset == 64) {
                bitOffset = 0;
                byteIndex++;
            }
        }
        return static_cast<int>(value);
    }
}

// calculates needed bitsize
int CFG::typeSize(int bits) {
    if (bits <= 8) {
        return 8;
    } else if (bits <= 16) {
        return 16;
    } else if (bits <= 32) {
        return 32;
    } else {
        return 64;
    }
}

// creates the respective bittype array
void* CFG::createArray(int type, int size) {
    switch (type) {
        case 8:
            return new uint8_t[size];
        case 16:
            return new uint16_t[size];
        case 32:
            return new uint32_t[size];
        case 64:
            return new uint64_t[size];
    }
}

// finds the most significant bit
int CFG::MSB(int num) {
    int msbPos = 0;
    while (num != 0) {
        num >>= 1;
        msbPos++;
    }
    return msbPos;
}

// both of these unused in this implementation
//std::string CFG::storeMSBnumber(int num) {
//
//    std::string binary = std::bitset<64>(num).to_string();
//    binary.erase(0, binary.find_first_not_of('0'));
//
//    return binary;
//}
//
//int CFG::MSBarr(const std::vector<int>& arr) {
//    int msb = 0;
//    for (int ele : arr) {
//        msb = std::max(msb, MSB(ele));
//    }
//    return msb;
//}

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

void CFG::reorderRules(uint64_t* ruleSizes)
{
    ruleLengths = new int[startRule+1];
    innerLengths = new int[startRule+1];
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

    // start by iterating over key/value pairs in sizeMap (so expansion length and respective count)
    for (auto& [size, count] : sizeMap) {
        // so if count is 2, we'll add 2 to nextOffset where the next rule size resides
        nextOffset += count;
        // make count the place where the last occurrence of the size resides
        count += offset;
        // prepare for next iteration
        offset = nextOffset;
    }

    // assign new rule characters using the last occurrences
    // newOrdering will store new indexes for each rule
    //void* newOrdering = new uint64_t [startRule + 1];
    // assign new rule characters using the last occurrences
    int* newOrdering = new int[startRule + 1];
    for (int i = CFG::ALPHABET_SIZE; i < startRule; i++) {
        size = ruleSizes[i];
        newOrdering[i] = sizeMap[size]--;
    }
    newOrdering[startRule] = startRule;

    // reorder the rules and update characters
    int** newRules2 = new int*[startRule + 1];
    int c2, newIndex2;
    for (int i = CFG::ALPHABET_SIZE; i <= startRule; i++) {
        for (int j = 0; (c2 = rules[i][j]) != CFG::DUMMY_CODE; j++) {
            if (c2 < CFG::ALPHABET_SIZE) {
                rules[i][j] = c2;
            } else {
                rules[i][j] = newOrdering[c2];
            }
        }
        newIndex2 = newOrdering[i];
        newRules2[newIndex2] = rules[i];
    }
    delete[] rules;
    rules = newRules2;

    int c;
    newRules = new void*[startRule + 1];

    //iterate through every lhs rule
    for (int i = CFG::ALPHABET_SIZE; i <= startRule; i++) {

        //count number of rhs rules for each lhs
        int ruleLength = 0;
        while (rules[i][ruleLength] != CFG::DUMMY_CODE) {
            ruleLength++;
        }
        //note, this does not include dummycode, as dummycodes are excluded from the bitpack
        ruleLengths[i] = ruleLength;

        int lhsMSB = MSB(i - 1); //most significant bit from i - 1 (ex 9 for 256 (100000000); -1 would give 8 for 255 (011111111)
        int bitSize = typeSize(lhsMSB); //smallest array size needed to store i (ex 16 bit array for 9 msb or 8 for 6, etc)
        if (bitSize == 8) {
            newRules[i] = new uint8_t[ruleLength];
        } else if (bitSize == 16) {
            newRules[i] = new uint16_t[ruleLength];
        } else if (bitSize == 32) {
            newRules[i] = new uint32_t[ruleLength];
        } else {
            newRules[i] = new uint64_t[ruleLength];
        }
        std::string finalBits; //for some bitshift testing purposes

        //this saves only the needed bits from each rhs value (001110 -> 1110) and combines all rhs values into a string for bitpacking
        for (int j = 0; j < ruleLength; j++) {

            c = rules[i][j];
            if (bitSize == 8) {
                uint8_t value = static_cast<uint8_t>(c);
                std::string bitString = std::bitset<8>(value).to_string();
                bitString.erase(0, 8-lhsMSB);
                //not used currently, but was for messing around with RHS MSB stuff, fills out 0's to correct size
                if (finalBits.length()+1 < lhsMSB && finalBits.length() != 0) {
                    finalBits.insert(0, lhsMSB-finalBits.length()+1, '0');
                }
                finalBits = finalBits + bitString;
            } else if (bitSize == 16) {
                uint16_t value = static_cast<uint16_t>(c);
                std::string bitString = std::bitset<16>(value).to_string();
                bitString.erase(0, 16-lhsMSB);
                if (finalBits.length()+1 < lhsMSB && finalBits.length() != 0) {
                    finalBits.insert(0, lhsMSB-finalBits.length()+1, '0');
                }
                finalBits = finalBits + bitString;
            } else if (bitSize == 32) {
                uint32_t value = static_cast<uint32_t>(c);
                std::string bitString = std::bitset<32>(value).to_string();
                bitString.erase(0, 32-lhsMSB);
                if (finalBits.length()+1 < lhsMSB && finalBits.length() != 0) {
                    finalBits.insert(0, lhsMSB-finalBits.length()+1, '0');
                }
                finalBits = finalBits + bitString;
            } else if (bitSize == 64) {
                uint64_t value = static_cast<uint64_t>(c);
                std::string bitString = std::bitset<64>(value).to_string();
                bitString.erase(0, 64-lhsMSB);
                if (finalBits.length()+1 < lhsMSB && finalBits.length() != 0) {
                    finalBits.insert(0, lhsMSB-finalBits.length()+1, '0');
                }
                finalBits = finalBits + bitString;
            }
        }

        //this calculates the smallest inner-array size required
        int sizeInnerArray;
        int sizeRemainder = finalBits.length() % bitSize;
        if (sizeRemainder != 0) {
            //fill out the last array if there aren't enough bits
            for (int r = 0; r < bitSize-sizeRemainder; r++) {
                finalBits.push_back('0');
            }
            sizeInnerArray = finalBits.length() / bitSize;
        } else {
            //get the exact number of elements needed for the inner array
            sizeInnerArray = finalBits.length() / bitSize;
        }
        innerLengths[i] = sizeInnerArray;
//        if (i == startRule) {
//            std::cout << "finalBits.length(): " << finalBits.length() << ", InnerArraySize: " << sizeInnerArray << std::endl;
//            std::cout << "bitSize: " << bitSize << ", sizeRemainder: " << sizeRemainder << std::endl;
//        }
        //creates the correct inner array size/type at newRules[i]
        newRules[i] = createArray(bitSize, sizeInnerArray);

        std::vector<std::string> finalBitsParts;
        //this gets correct segments and stuffs them in the array. Could be much faster without strings, but not as important since the pack doesn't change
        for (int p = 0; p < sizeInnerArray; p++) {
            std::string finalBitsSegments = finalBits.substr(p*bitSize, bitSize);
            uint64_t segmentStorage = std::bitset<64>(finalBitsSegments).to_ullong();

            if (bitSize == 8) {
                static_cast<uint8_t*>(newRules[i])[p] = static_cast<uint8_t>(segmentStorage);
            } else if (bitSize == 16) {
                static_cast<uint16_t*>(newRules[i])[p] = static_cast<uint16_t>(segmentStorage);
            } else if (bitSize == 32) {
                static_cast<uint32_t*>(newRules[i])[p] = static_cast<uint32_t>(segmentStorage);
            } else {
                static_cast<uint64_t *>(newRules[i])[p] = static_cast<uint64_t>(segmentStorage);
            }
        }
    }

    delete[] newOrdering;
}

void CFG::postProcess()
{
    // prepare post-processing structures
    // uint array of startrule size, for test example ~ 2072 bytes of memory (259 * 8)
    uint64_t* ruleSizes = new uint64_t[startRule + 1];
    // int array of startrule size (each element 64 bit uint)
    int* ruleDepths = new int[startRule + 1];
    // intializing every element (up to 256) in ruleSizes and ruleDepths arrays to 1
    for (int i = 0; i < CFG::ALPHABET_SIZE; i++) {
        ruleSizes[i] = 1;
        ruleDepths[i] = 1;
    }
    //sets values past 256 (rules and startrule) to 0
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

    // order the rules by expansion length; shortest to longest
    reorderRules(ruleSizes);

    // clean up rule sizes
    delete[] ruleSizes;
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
    cfg->postProcess();

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
    cfg->postProcess();

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
    unsigned int len = s.st_size;

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
    cfg->postProcess();

    return cfg;
}

}