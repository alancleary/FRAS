#ifndef INCLUDED_AMT_SET
#define INCLUDED_AMT_SET

#include <cstdint>

namespace amt {

/** A set implemented as an array mapped trie. */
class Set
{

private:

    // maximum node size is 1 (bitMap) + 64 (child pointers or leaf values) + 1 as arrays are zero based
    static const int FREE_LIST_SIZE = 1+64+1;

    static const int KNOWN_EMPTY_NODE = 0;
    static const int KNOWN_DELETED_NODE = 1;
    static const int HEADER_SIZE = 2;  // KNOWN_EMPTY_NODE, KNOWN_DELETED_NODE

    int currentSize;
    //uint64_t* mem;
    uint64_t* freeLists;
    //uint64_t freeIdx;

    //uint64_t root;
    uint64_t count;

    uint64_t allocate(int size);
    uint64_t allocateInsert(uint64_t nodeIdx, int size, int childIdx);
    uint64_t allocateDelete(uint64_t nodeIdx, int size, int childIdx);
    void deallocate(uint64_t idx, int size);

    uint64_t createLeaf(uint8_t* key, int off, int len);
    uint64_t insertChild(uint64_t nodeRef, uint64_t bitMap, uint64_t bitPos, int idx, uint64_t value);
    uint64_t removeChild(uint64_t nodeRef, uint64_t bitMap, uint64_t bitPos, int idx);

    uint64_t set(uint64_t nodeRef, uint8_t* key, int off, int len);
    bool predecessor(uint64_t nodeRef, uint8_t* key, int off, int len);
    bool successor(uint64_t nodeRef, uint8_t* key, int off, int len);
    uint64_t clear(uint64_t nodeRef, uint8_t* key, int off, int len);

    uint64_t lowestOneBit(uint64_t value);
    uint64_t highestOneBit(uint64_t value);

public:

    void tmp(uint64_t nodeRef, uint8_t* key, int off, int len);

    // TODO: make private
    uint64_t* mem;
    uint64_t root;
    uint64_t freeIdx;

    Set(int size);
    ~Set();

    uint64_t size() { return count; };

    /**
      * Adds the given uint8_t key to the set.
      *
      * @param key The uint8_t key.
      * @param len The length of the uint8_t key.
      * @return A bool saying whether or not the key was added.
      */
    bool set(uint8_t* key, int len);

    /**
      * Checks if the given uint8_t key exists in the set.
      *
      * @param key The uint8_t key.
      * @param len The length of the uint8_t key.
      * @return A bool saying whether or not the key exists.
      */
    bool get(uint8_t* key, int len);

    /**
      * Gets the largest key that is less than or equal to the given uint8_t key. This is equivalent
      * to a paired rank-select query on a bit vector, i.e. select(rank(key)).
      *
      * @param key The uint8_t key to match that will be updated if a different key is selected.
      * @param len The length of the uint8_t key.
      * @return A bool saying whether or not a key was selected.
      */
    bool predecessor(uint8_t* key, int len);

    /**
      * Gets the smallest key that is greater than or equal to the given uint8_t key. This is equivalent
      * to a paired rank-select query on a bit vector, i.e. select(rank(key) + 1).
      *
      * @param key The uint8_t key to match that will be updated if a different key is selected.
      * @param len The length of the uint8_t key.
      * @return A bool saying whether or not a key was selected.
      */
    bool successor(uint8_t* key, int len);

    /**
      * Removes the given uint8_t key from the set.
      *
      * @param key The uint8_t key.
      * @param len The length of the uint8_t key.
      * @return A bool saying whether or not the key was removed.
      */
    bool clear(uint8_t* key, int len);

};

}

#endif
