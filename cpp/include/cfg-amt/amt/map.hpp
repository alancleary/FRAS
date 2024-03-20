#ifndef INCLUDED_CFG_AMT_AMT_MAP
#define INCLUDED_CFG_AMT_AMT_MAP

#include <cstdint>

namespace cfg_amt {

class MapVisitor
{
public:
    virtual void visit(uint8_t* key, int keyLen, uint64_t value) = 0;
};

/** A map implemented as a bitwise trie with a bitmap. */
class Map
{

private:

    // maximum node size is 1 (bitMap) + 64 (child pointers or leaf values) + 1 as arrays are zero based
    static const int FREE_LIST_SIZE = 1+64+1;

    static const int KNOWN_EMPTY_NODE = 0;
    static const int KNOWN_DELETED_NODE = 1;
    static const int HEADER_SIZE = 2;  // KNOWN_EMPTY_NODE, KNOWN_DELETED_NODE

    int currentSize;
    uint64_t* mem;
    uint64_t* freeLists;
    uint64_t freeIdx;

    uint64_t root;
    uint64_t count;
    uint64_t nodeCount;

    uint64_t allocate(int size);
    uint64_t allocateInsert(uint64_t nodeIdx, int size, int childIdx);
    uint64_t allocateDelete(uint64_t nodeIdx, int size, int childIdx);
    void deallocate(uint64_t idx, int size);

    uint64_t createLeaf(uint8_t* key, int off, int len, uint64_t keyValue);
    uint64_t insertChild(uint64_t nodeRef, uint64_t bitMap, uint64_t bitPos, int idx, uint64_t value);
    uint64_t removeChild(uint64_t nodeRef, uint64_t bitMap, uint64_t bitPos, int idx);

    uint64_t set(uint64_t nodeRef, uint8_t* key, int off, int len, uint64_t keyValue);
    uint64_t predecessor(uint64_t nodeRef, uint8_t* key, int off, int len);
    uint64_t successor(uint64_t nodeRef, uint8_t* key, int off, int len);
    uint64_t clear(uint64_t nodeRef, uint8_t* key, int off, int len);

    uint64_t lowestOneBit(uint64_t value);
    uint64_t highestOneBit(uint64_t value);

    void visit(MapVisitor& visitor, int len, uint64_t nodeRef, uint8_t* key, int off);
    void visitRange(MapVisitor& visitor, uint32_t begin, uint32_t end, int len, uint8_t* key, int pos, uint64_t nodeRef, int off);

public:

    Map(int size);
    ~Map();

    uint64_t size() { return count; }
    uint64_t nodeSize() { return nodeCount; }

    /**
      * Adds the given key-value to the map or updates the value if the key already exists.
      *
      * @param key The uint8_t key.
      * @param len The length of the uint8_t key.
      * @param keyValue The value to store for the uint8_t key.
      * @return A bool saying whether or not the key was added.
      */
    bool set(uint8_t* key, int len, uint64_t keyValue);

    /**
      * Checks if the given uint8_t key exists in the set.
      *
      * @param key The uint8_t key.
      * @param len The length of the uint8_t key.
      * @return The value stored for the given key.
      * @throws Exception if the key is not found.
      */
    uint64_t get(uint8_t* key, int len);

    /**
      * Selects the largest key that is less than or equal to the given uint8_t key and returns its
      * value. This is equivalent to a paired rank-select query on a bit vector, i.e.
      * select(rank(key)).
      *
      * @param key The uint8_t key to match that will be updated if a different key is selected.
      * @param len The length of the uint8_t key.
      * @return The value stored for the selected key.
      * @throws Exception if a key is not selected.
      */
    uint64_t predecessor(uint8_t* key, int len);

    /**
      * Selects the smallest key that is greater than or equal to the given uint8_t key and returns
      * its value. This is equivalent to a paired rank-select query on a bit vector, i.e.
      * select(rank(key) + 1).
      *
      * @param key The uint8_t key to match that will be updated if a different key is selected.
      * @param len The length of the uint8_t key.
      * @return The value stored for the selected key.
      * @throws Exception if a key is not selected.
      */
    uint64_t successor(uint8_t* key, int len);

    /**
      * Removes the given uint8_t key from the set.
      *
      * @param key The uint8_t key.
      * @param len The length of the uint8_t key.
      * @return A bool saying whether or not the key was removed.
      */
    bool clear(uint8_t* key, int len);

    /**
      * Visitirs every key-value pair in the map.
      * @param visitor The class that will visit every key-value pair.
      * @param len The length of keys in the map.
      */
    void visit(MapVisitor& visitor, int len);

    /**
      * Visits every key-value pair in the map in a specific key range.
      * @param visitor The class that will visit every key-value pair.
      * @param begin The beginning of the range to visit.
      * @param end The end of the range to visit.
      * @param len The length of keys in the map.
      */
    void visitRange(MapVisitor& visitor, uint32_t begin, uint32_t end, int len);

    /**
      * Visits every key-value pair in the map in a specific key range.
      * @param visitor The class that will visit every key-value pair.
      * @param begin The beginning of the range to visit.
      * @param end The end of the range to visit.
      * @param len The length of keys in the map.
      * @param key The key to use.
      * @param pos The position to use in the key.
      */
    void visitRange(MapVisitor& visitor, uint32_t begin, uint32_t end, int len, uint8_t* key, int pos);

};

}

#endif
