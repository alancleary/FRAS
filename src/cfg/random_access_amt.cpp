#include <cstddef>
#include "amt/key.hpp"
#include "amt/set.hpp"
#include "cfg/cfg.hpp"
#include "cfg/jagged_array_int.hpp"
#include "cfg/random_access_amt.hpp"

namespace cfg {

// static

template <class CFG_T>
uint64_t RandomAccessAMT<CFG_T>::getKey(uint8_t* key)
{
    return (uint64_t) amt::get6Int(key);
}

template <class CFG_T>
void RandomAccessAMT<CFG_T>::setKey(uint8_t* key, uint64_t value)
{
    amt::set6Int(key, (uint32_t) value);
}

// construction

template <class CFG_T>
RandomAccessAMT<CFG_T>::RandomAccessAMT(CFG_T* cfg): RandomAccess<CFG_T>(cfg)
{
    amt::Set set(1024);
    setValues(set);
    cset = new amt::CompressedSumSet(set, 6, RandomAccessAMT::getKey, RandomAccessAMT::setKey);
}

// deconstruction

template <class CFG_T>
RandomAccessAMT<CFG_T>::~RandomAccessAMT()
{
    delete cset;
}

// private

template <class CFG_T>
void RandomAccessAMT<CFG_T>::setValues(amt::Set& set)
{
    uint64_t* ruleSizes = new uint64_t[this->cfg->getStartRule()];
    for (int i = 0; i < CFG_T::ALPHABET_SIZE; i++) {
        ruleSizes[i] = 1;
    }
    for (int i = CFG_T::ALPHABET_SIZE; i < this->cfg->getStartRule(); i++) {
        ruleSizes[i] = 0;
    }
    uint8_t* key = new uint8_t[RandomAccessAMT::KEY_LENGTH];
    int len;
    uint64_t pos = 0;
    int c;
    for (int i = 0; i < this->cfg->getStartSize(); i++) {
        c = this->cfg->get(this->cfg->getStartRule(), i);
        len = amt::set6Int(key, pos);
        set.set(key, len);
        pos += ruleSize(ruleSizes, c);
    }
    delete[] ruleSizes;
    delete[] key;
}

template <class CFG_T>
uint64_t RandomAccessAMT<CFG_T>::ruleSize(uint64_t* ruleSizes, int rule)
{
    if (ruleSizes[rule] != 0) return ruleSizes[rule];

    int c;
    for (int i = 0; (c = this->cfg->get(rule, i)) != CFG_T::DUMMY_CODE; i++) {
        if (ruleSizes[c] == 0) {
            ruleSize(ruleSizes, c);
        }
        ruleSizes[rule] += ruleSizes[c];
    }

    return ruleSizes[rule];
}

template <class CFG_T>
void RandomAccessAMT<CFG_T>::rankSelect(uint64_t i, int& rank, uint64_t& select)
{
    uint8_t* key = new uint8_t[KEY_LENGTH];
    // i+1 because rank is exclusive [0, i) and we want inclusive [0, i]
    amt::set6Int(key, (uint32_t) i + 1);
    rank = (int) cset->predecessor(key, KEY_LENGTH);
    select = (uint64_t) amt::get6Int(key);
    delete[] key;
}

// instantiate the class
template class RandomAccessAMT<CFG<JaggedArrayInt>>;

}
