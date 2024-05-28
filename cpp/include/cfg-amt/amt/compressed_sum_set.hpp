#ifndef INCLUDED_CFG_AMT_COMPRESSED_SUM_SET
#define INCLUDED_CFG_AMT_COMPRESSED_SUM_SET

#include <cstdint>
#include <functional>  // function
#include "cfg-amt/amt/set.hpp"

namespace cfg_amt {

/**
 * A set implemented as an array mapped trie (AMT) with tail compression and a partial sum of the number of
 * values that preceed each leaf node.
 *
 * The structure is built from an existing AMT Set and is immutable.
 **/
class CompressedSumSet
{

private:

    typedef std::function<uint64_t(uint8_t*)> GetKey;
    typedef std::function<void(uint8_t*, uint64_t)> SetKey;

    static const int KNOWN_EMPTY_NODE = 0;

    GetKey getKey;
    SetKey setKey;

    int leafCount;
    int memSize;
    uint64_t* mem;
    bool* compressed;

    uint64_t root;
    uint64_t count;

    void computeCompressedSize(Set& set, int len);
    int computeCompressedSize(Set& set, int len, uint64_t nodeRef, int off);
    void construct(Set& set, int len);
    int construct(Set& set, int len, uint64_t nodeRef, uint8_t* key, int off, int& idx, int& sum);

    uint64_t predecessor(uint64_t nodeRef, uint8_t* key, int off, int len);
    //bool successor(uint64_t nodeRef, uint8_t* key, int off, int len);

    //uint64_t lowestOneBit(uint64_t value);
    //uint64_t highestOneBit(uint64_t value);

    void tmp(uint64_t nodeRef, uint8_t* key, int off, int len);

public:

    void tmp(int len);

    CompressedSumSet(Set& set, int len, GetKey getKey, SetKey setKey);
    ~CompressedSumSet();

    uint64_t size() { return count; };

    /**
      * Checks if the given uint8_t key exists in the set.
      *
      * @param key The uint8_t key.
      * @param len The length of the uint8_t key.
      * @return A bool saying whether or not the key exists.
      */
    //bool get(uint8_t* key, int len);

    /**
      * Gets the largest key that is less than or equal to the given uint8_t key. This is equivalent
      * to a paired rank-select query on a bit vector, i.e. select(rank(key)).
      *
      * @param key The uint8_t key to match that will be updated if a different key is selected.
      * @param len The length of the uint8_t key.
      * @return A bool saying whether or not a key was selected.
      */
    //bool predecessor(uint8_t* key, int len);

    /**
      * Selects the largest key that is less than or equal to the given uint8_t key and returns its
      * rank. This is equivalent to a paired rank-select query on a bit vector, i.e.
      * select(rank(key)).
      *
      * @param key The uint8_t key to match that will be updated if a different key is selected.
      * @param len The length of the uint8_t key.
      * @return The rank of the selected key.
      * @throws Exception if a key is not selected.
      */
    uint64_t predecessor(uint8_t* key, int len);

    /**
      * Gets the smallest key that is greater than or equal to the given uint8_t key. This is equivalent
      * to a paired rank-select query on a bit vector, i.e. select(rank(key) + 1).
      *
      * @param key The uint8_t key to match that will be updated if a different key is selected.
      * @param len The length of the uint8_t key.
      * @return A bool saying whether or not a key was selected.
      */
    //bool successor(uint8_t* key, int len);

};

}

#endif
