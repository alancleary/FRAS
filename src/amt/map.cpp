#include <algorithm>
#include <bit>
#include <stdexcept>
#include "amt/bitops.hpp"
#include "amt/key.hpp"
#include "amt/map.hpp"

namespace amt {

// construction

Map::Map(int size): currentSize(size)
{
    mem = new uint64_t[currentSize];
    freeLists = new uint64_t[Map::FREE_LIST_SIZE];
    for (int i = 0; i < Map::FREE_LIST_SIZE; i++) {
        freeLists[i] = 0;
    }
    freeIdx = Map::HEADER_SIZE;
    root = Map::KNOWN_EMPTY_NODE;
    count = 0;
    nodeCount = 0;
}

// destruction

Map::~Map()
{
    delete[] mem;
    delete[] freeLists;
}

// memory management

uint64_t Map::allocate(int size)
{
    uint64_t free = freeLists[size];
    if (free != 0) {
        // requested size available in free list, re-link and return head
        freeLists[size] = mem[(int) free];
        return free;
    } else {
        // expansion required?
        if (freeIdx + size > currentSize) {
            // increase by 25% and assure this is enough
            int newSize = currentSize + std::max(currentSize / 4, size);
            uint64_t* newMem = new uint64_t[newSize];
            for (int i = 0; i < currentSize; i++) {
                newMem[i] = mem[i];
            }
            delete[] mem;
            mem = newMem;
            currentSize = newSize;
        }
        uint64_t idx = freeIdx;
        freeIdx += size;
        return idx;
    }
}

uint64_t Map::allocateInsert(uint64_t nodeIdx, int size, int childIdx)
{
    uint64_t newNodeRef = allocate(size + 1);

    int a = (int) newNodeRef;
    int b = (int) nodeIdx;

    // copy with gap for child
    for (int j = 0; j < childIdx; j++) {
        mem[a++] = mem[b++];
    }
    a++; // inserted
    for (int j = childIdx; j < size; j++) {
        mem[a++] = mem[b++];
    }

    deallocate(nodeIdx, size);

    return newNodeRef;
}

uint64_t Map::allocateDelete(uint64_t nodeIdx, int size, int childIdx)
{
    uint64_t newNodeRef = allocate(size - 1);

    // copy with child removed
    int a = (int) newNodeRef;
    int b = (int) nodeIdx;
    for (int j = 0; j < childIdx; j++) {
        mem[a++] = mem[b++];
    }
    b++; // removed
    for (int j = childIdx + 1; j < size; j++) {
        mem[a++] = mem[b++];
    }

    deallocate(nodeIdx, size);

    return newNodeRef;
}

void Map::deallocate(uint64_t idx, int size)
{
    if (idx == Map::KNOWN_EMPTY_NODE) {
        return;  // keep our known empty node
    }

    // add to head of free-list
    mem[(int) idx] = freeLists[size];
    freeLists[size] = idx;
}

// tree structure

uint64_t Map::createLeaf(uint8_t* key, int off, int len, uint64_t keyValue)
{
    uint64_t newNodeRef = allocate(2);
    int a = (int) newNodeRef;
    mem[a++] = ((uint64_t) 1) << key[len - 1];
    mem[a] = keyValue;
    nodeCount += 2;
    len -= 2;
    while (len >= off) {
        uint64_t newParentNodeRef = allocate(2);
        a = (int) newParentNodeRef;
        mem[a++] = ((uint64_t) 1) << key[len--];
        mem[a] = newNodeRef;
        nodeCount += 2;
        newNodeRef = newParentNodeRef;
    }
    return newNodeRef;
}

uint64_t Map::insertChild(uint64_t nodeRef, uint64_t bitMap, uint64_t bitPos, int idx, uint64_t value)
{
    int size = std::popcount(bitMap);
    uint64_t newNodeRef = allocateInsert(nodeRef, size + 1, idx + 1);
    mem[(int) newNodeRef] = bitMap | bitPos;
    mem[(int) newNodeRef + 1 + idx] = value;
    nodeCount += 1;
    return newNodeRef;
}

uint64_t Map::removeChild(uint64_t nodeRef, uint64_t bitMap, uint64_t bitPos, int idx)
{
    int size = std::popcount(bitMap);
    if (size > 1) {
        // node still has other children / leaves
        uint64_t newNodeRef = allocateDelete(nodeRef, size + 1, idx + 1);
        mem[(int) newNodeRef] = bitMap & ~bitPos;
        return newNodeRef;
    } else {
        // node is now empty, remove it
        deallocate(nodeRef, size + 1);
        nodeCount -= 1;
        return Map::KNOWN_DELETED_NODE;
    }
}

// associative array operations

bool Map::set(uint8_t* key, int len, uint64_t keyValue)
{
    uint64_t nodeRef = set(root, key, 0, len, keyValue);
    if (nodeRef != Map::KNOWN_EMPTY_NODE) {
        // denotes change
        count++;
        root = nodeRef;
        return true;
    } else {
        return false;
    }
}

uint64_t Map::set(uint64_t nodeRef, uint8_t* key, int off, int len, uint64_t keyValue)
{
    uint64_t bitMap = mem[(int) nodeRef];
    uint64_t bitPos = ((uint64_t) 1) << key[off++];  // mind the ++
    int idx = std::popcount(bitMap & (bitPos - 1));

    if ((bitMap & bitPos) == 0) {
        // child not present yet
        uint64_t value;
        if (off == len) {
            value = keyValue;
        } else {
            value = createLeaf(key, off, len, keyValue);
        }
        return insertChild(nodeRef, bitMap, bitPos, idx, value);
    } else {
        // child present
        if (off == len) {
            mem[(int) nodeRef + 1 + idx] = keyValue;
            return Map::KNOWN_EMPTY_NODE;
        } else {
            // not at leaf, recursion
            uint64_t childNodeRef = mem[(int) nodeRef + 1 + idx];
            uint64_t newChildNodeRef = set(childNodeRef, key, off, len, keyValue);
            if (newChildNodeRef == Map::KNOWN_EMPTY_NODE) {
                return Map::KNOWN_EMPTY_NODE;
            }
            if (newChildNodeRef != childNodeRef) {
                mem[(int) nodeRef + 1 + idx] = newChildNodeRef;
            }
            return nodeRef;
        }
    }
}

uint64_t Map::get(uint8_t* key, int len)
{
    if (root == Map::KNOWN_EMPTY_NODE) {
        throw std::runtime_error("Key not found");
    }

    uint64_t nodeRef = root;
    int off = 0;

    for (;;) {
        uint64_t bitMap = mem[(int) nodeRef];
        uint64_t bitPos = ((uint64_t) 1) << key[off++];  // mind the ++
        if ((bitMap & bitPos) == 0) {
            throw std::runtime_error("Key not found");
        }

        uint64_t value = mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
        if (off == len) {
            return value;
        } else {
            // child pointer
            nodeRef = value;
        }
    }
}

uint64_t Map::predecessor(uint8_t* key, int len)
{
    if (root == Map::KNOWN_EMPTY_NODE) {
        throw std::runtime_error("No key to select");
    }

    uint64_t nodeRef = root;
    int off = 0;

    uint64_t nearestNodeRef = Map::KNOWN_EMPTY_NODE;
    int nearestOff = 0;

    for (;;) {
        uint64_t bitMap = mem[(int) nodeRef];
        uint64_t bitPos = ((uint64_t) 1) << key[off];

        // memoize the node if it has smaller keys
        if (amt::lowestOneBit(bitMap) < bitPos) {
            nearestNodeRef = nodeRef;
            nearestOff = off;
        }

        // key not found
        if ((bitMap & bitPos) == 0) {
            return predecessor(nearestNodeRef, key, nearestOff, len);
        }

        uint64_t idx = nodeRef + 1 + std::popcount(bitMap & (bitPos - 1));
        uint64_t value = mem[(int) idx];

        if (++off == len) {
            // at value
            return value;
        } else {
            // child pointer
            nodeRef = value;
        }
    }
}

uint64_t Map::predecessor(uint64_t nodeRef, uint8_t* key, int off, int len)
{
    // no smaller key exists
    if (nodeRef == Map::KNOWN_EMPTY_NODE) {
        throw std::runtime_error("No key to select");
    }

    uint64_t bitMap = mem[(int) nodeRef];
    uint64_t bitPos = ((uint64_t) 1) << key[off];

    // get the largest key that is less than the given key
    uint64_t maskedBitMap = bitMap & (bitPos - 1);
    key[off] = largestKey(maskedBitMap);
    bitPos = ((uint64_t) 1) << key[off++];  // mind the ++

    // get the largest key in all remaining nodes
    nodeRef = mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
    while (off < len) {
        bitMap = mem[(int) nodeRef];
        key[off] = largestKey(bitMap);
        bitPos = ((uint64_t) 1) << key[off++];  // mind the ++
        nodeRef = mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
    }

    return nodeRef;
}

uint64_t Map::successor(uint8_t* key, int len)
{
    if (root == Map::KNOWN_EMPTY_NODE) {
        throw std::runtime_error("No key to select");
    }

    uint64_t nodeRef = root;
    int off = 0;

    uint64_t nearestNodeRef = Map::KNOWN_EMPTY_NODE;
    int nearestOff = 0;

    for (;;) {
        uint64_t bitMap = mem[(int) nodeRef];
        uint64_t bitPos = ((uint64_t) 1) << key[off];

        // memoize the node if it has larger keys
        if (amt::highestOneBit(bitMap) > bitPos) {
            nearestNodeRef = nodeRef;
            nearestOff = off;
        }

        // key not found
        if ((bitMap & bitPos) == 0) {
            return successor(nearestNodeRef, key, nearestOff, len);
        }

        uint64_t idx = nodeRef + 1 + std::popcount(bitMap & (bitPos - 1));
        uint64_t value = mem[(int) idx];

        if (++off == len) {
            // at leaf
            return value;
        } else {
            // child pointer
            nodeRef = value;
        }
    }
}

uint64_t Map::successor(uint64_t nodeRef, uint8_t* key, int off, int len)
{
    // no smaller key exists
    if (nodeRef == Map::KNOWN_EMPTY_NODE) {
        throw std::runtime_error("No key to select");
    }

    uint64_t bitMap = mem[(int) nodeRef];
    uint64_t bitPos = ((uint64_t) 1) << (key[off] + 1);  // +1 because bitMap is exclusive

    // get the smallest key that is greater than the given key
    uint64_t maskedBitMap = bitMap & ~(bitPos - 1);
    key[off] = smallestKey(maskedBitMap);
    bitPos = ((uint64_t) 1) << key[off++];  // mind the ++

    // get the smallest key in all remaining nodes
    nodeRef = mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
    while (off < len) {
        bitMap = mem[(int) nodeRef];
        key[off] = smallestKey(bitMap);
        bitPos = ((uint64_t) 1) << key[off++];  // mind the ++
        nodeRef = mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
    }

    return nodeRef;
}

bool Map::clear(uint8_t* key, int len)
{
    uint64_t nodeRef = clear(root, key, 0, len);
    if (nodeRef != Map::KNOWN_EMPTY_NODE) {
        count--;
        if (nodeRef == Map::KNOWN_DELETED_NODE) {
            root = Map::KNOWN_EMPTY_NODE;
        } else {
            root = nodeRef;
        }
        return true;
    } else {
        return false;
    }
}

uint64_t Map::clear(uint64_t nodeRef, uint8_t* key, int off, int len)
{
    if (root == Map::KNOWN_EMPTY_NODE) {
        return Map::KNOWN_EMPTY_NODE;
    }

    uint64_t bitMap = mem[(int) nodeRef];
    uint64_t bitPos = ((uint64_t) 1) << key[off++];  // mind the ++
    if ((bitMap & bitPos) == 0) {
        // child not present, key not found
        return Map::KNOWN_EMPTY_NODE;
    } else {
        // child present
        int idx = std::popcount(bitMap & (bitPos - 1));
        if (off == len) {
            // remove the stored value
            return removeChild(nodeRef, bitMap, bitPos, idx);
        } else {
            // not at leaf
            uint64_t childNodeRef = mem[(int) nodeRef + 1 + idx];
            uint64_t newChildNodeRef = clear(childNodeRef, key, off, len);
            if (newChildNodeRef == Map::KNOWN_EMPTY_NODE) {
                return Map::KNOWN_EMPTY_NODE;
            }
            if (newChildNodeRef == Map::KNOWN_DELETED_NODE) {
                return removeChild(nodeRef, bitMap, bitPos, idx);
            }
            if (newChildNodeRef != childNodeRef) {
                mem[(int) nodeRef + 1 + idx] = newChildNodeRef;
            }
            return nodeRef;
        }
    }
}

// visiting

void Map::visit(MapVisitor& visitor, int len)
{
    uint8_t* key = new uint8_t[64];
    visit(visitor, len, root, key, 0);
    delete[] key;
}

void Map::visit(MapVisitor& visitor, int len, uint64_t nodeRef, uint8_t* key, int off)
{
    uint64_t bitMap = mem[(int) nodeRef];
    uint64_t bits = bitMap;
    while (bits != 0) {
        uint64_t bitPos = bits & -bits; bits ^= bitPos;  // get rightmost bit and clear it
        int bitNum = std::countr_zero(bitPos);
        key[off] = (uint8_t) bitNum;

        uint64_t value = mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];

        if (off == len - 1) {
            visitor.visit(key, len, value);
        } else {
            visit(visitor, len, value, key, off + 1);
        }
    }
}

void Map::visitRange(MapVisitor& visitor, uint32_t begin, uint32_t end, int len)
{
    uint8_t* key = new uint8_t[64];
    visitRange(visitor, begin, end, len, key, 0);
    delete[] key;
}

void Map::visitRange(MapVisitor& visitor, uint32_t begin, uint32_t end, int len, uint8_t* key, int pos)
{
    visitRange(visitor, begin, end, len, key, pos, root, 0);
}

void Map::visitRange(MapVisitor& visitor, uint32_t begin, uint32_t end, int len, uint8_t* key, int pos, uint64_t nodeRef, int off)
{
    // compute key[off] parts for begin and end
    int level = len - (off + 1);
    uint8_t x = (uint8_t) (begin >> (level * 6)) & 0x3F;
    uint8_t y = (uint8_t) (end >> (level * 6)) & 0x3F;

    // create a bitmap for masking values outside the range
    uint64_t bitMask = ((uint64_t) 1) << y;
    bitMask |= bitMask - 1;
    bitMask &= ~((((uint64_t) 1) << x) - 1);

    uint64_t bitMap = mem[(int) nodeRef];
    uint64_t bits = bitMap & bitMask;
    while (bits != 0) {
        uint64_t bitPos = bits & -bits; bits ^= bitPos;  // get rightmost bit and clear it
        uint8_t bitNum = std::countr_zero(bitPos);
        key[pos + off] = bitNum;

        uint64_t value = mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];

        if (off == len - 1) {
            visitor.visit(key, len, value);
        } else {
            if (x == y) {
                visitRange(visitor, begin, end, len, key, pos, value, off + 1);
            } else if (bitNum == x) {
                visitRange(visitor, begin, ~0x0, len, key, pos, value, off + 1);
            } else if (bitNum == y) {
                visitRange(visitor, 0, end, len, key, pos, value, off + 1);
            } else {
                visitRange(visitor, 0, ~0x0, len, key, pos, value, off + 1);
            }
        }
    }
}

void Map::visitTails(MapTailVisitor& visitor, int len)
{
    uint8_t* key = new uint8_t[64];
    visitTails(visitor, len, root, key, 0, 0);
    delete[] key;
}

void Map::visitTails(MapTailVisitor& visitor, int len, uint64_t nodeRef, uint8_t* key, int off, int tailLen)
{
    uint64_t bitMap = mem[(int) nodeRef];
    if (std::popcount(bitMap) > 1) {
        tailLen += 1;
    } else {
        tailLen = 0;
    }
    uint64_t bits = bitMap;
    while (bits != 0) {
        uint64_t bitPos = bits & -bits; bits ^= bitPos;  // get rightmost bit and clear it
        int bitNum = std::countr_zero(bitPos);
        key[off] = (uint8_t) bitNum;

        uint64_t value = mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];

        if (off == len - 1) {
            visitor.visit(key, len, value, tailLen);
        } else {
            visitTails(visitor, len, value, key, off + 1, tailLen);
        }
    }
}

}
