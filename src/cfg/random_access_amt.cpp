#include "amt/key.hpp"
#include "amt/set.hpp"
#include "cfg/random_access_amt.hpp"

namespace cfg {

// static

uint64_t RandomAccessAMT::getKey(uint8_t* key)
{
    return (uint64_t) amt::get6Int(key);
}

void RandomAccessAMT::setKey(uint8_t* key, uint64_t value)
{
    amt::set6Int(key, (uint32_t) value);
}

// construction

RandomAccessAMT::RandomAccessAMT(CFG* cfg): RandomAccess(cfg)
{
    amt::Set set(1024);
    setValues(set);
    cset = new amt::CompressedSumSet(set, 6, RandomAccessAMT::getKey, RandomAccessAMT::setKey);
}

// deconstruction

RandomAccessAMT::~RandomAccessAMT()
{
    delete cset;
}

// private

void RandomAccessAMT::setValues(amt::Set& set)
{
    uint64_t* ruleSizes = new uint64_t[cfg->startRule];
    for (int i = 0; i < CFG::ALPHABET_SIZE; i++) {
        ruleSizes[i] = 1;
    }
    for (int i = CFG::ALPHABET_SIZE; i < cfg->startRule; i++) {
        ruleSizes[i] = 0;
    }
    uint8_t* key = new uint8_t[RandomAccessAMT::KEY_LENGTH];
    int len;
    uint64_t pos = 0;
    int c;
    for (int i = 0; i < cfg->startSize; i++) {
        c = cfg->rules[cfg->startRule][i];
        len = amt::set6Int(key, pos);
        set.set(key, len);
        pos += ruleSize(ruleSizes, c);
    }
    delete[] ruleSizes;
    delete[] key;
}

uint64_t RandomAccessAMT::ruleSize(uint64_t* ruleSizes, int rule)
{
    if (ruleSizes[rule] != 0) return ruleSizes[rule];

    int c;
    for (int i = 0; (c = cfg->rules[rule][i]) != CFG::DUMMY_CODE; i++) {
        if (ruleSizes[c] == 0) {
            ruleSize(ruleSizes, c);
        }
        ruleSizes[rule] += ruleSizes[c];
    }

    return ruleSizes[rule];
}

void RandomAccessAMT::rankSelect(uint64_t i, int& rank, uint64_t& select)
{
    uint8_t* key = new uint8_t[KEY_LENGTH];
    // i+1 because rank is exclusive [0, i) and we want inclusive [0, i]
    amt::set6Int(key, (uint32_t) i + 1);
    rank = (int) cset->predecessor(key, KEY_LENGTH);
    select = (uint64_t) amt::get6Int(key);
    delete[] key;
}

}
