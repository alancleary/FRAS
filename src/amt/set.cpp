#include <algorithm>
#include <bit>
#include "amt/bitops.hpp"
#include "amt/key.hpp"
#include "amt/set.hpp"

#include <iostream>

namespace amt {

void Set::tmp(uint64_t nodeRef, uint8_t* key, int off, int len)
{
    uint64_t bitMap = mem[(int) nodeRef];
    if (bitMap == 0) {
        return;
    }
    uint64_t bits = bitMap;
    while (bits != 0) {
        uint64_t bitPos = bits & -bits; bits ^= bitPos; // get rightmost bit and clear it
        int bitNum = std::countr_zero(bitPos);
        key[off] = (uint8_t) bitNum;
        if (off == len - 2) {
            uint64_t value = mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
            uint64_t bits2 = value;
            while (bits2 != 0) {
                uint64_t bitPos2 = bits2 & -bits2; bits2 ^= bitPos2;
                int bitNum2 = std::countr_zero(bitPos2);
                key[off + 1] = (uint8_t) bitNum2;
                std::cerr << "key: " << get6Int(key) << std::endl;
            }
        } else {
            uint64_t childNode = mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
            tmp(childNode, key, off + 1, len);
        }
    }
}

// construction

Set::Set(int size): currentSize(size) {
    mem = new uint64_t[currentSize];
    for (int i = 0; i < currentSize; i++) {
        mem[i] = 0;
    }
    freeLists = new uint64_t[Set::FREE_LIST_SIZE];
    for (int i = 0; i < Set::FREE_LIST_SIZE; i++) {
        freeLists[i] = 0;
    }
    freeIdx = Set::HEADER_SIZE;
    root = Set::KNOWN_EMPTY_NODE;
    count = 0;
}

// destruction

Set::~Set() {
    delete[] mem;
    delete[] freeLists;
}

// memory management

uint64_t Set::allocate(int size) {
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
            for (int i = currentSize; i < newSize; i++) {
                newMem[i] = 0;
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

uint64_t Set::allocateInsert(uint64_t nodeIdx, int size, int childIdx) {
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

uint64_t Set::allocateDelete(uint64_t nodeIdx, int size, int childIdx) {
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

void Set::deallocate(uint64_t idx, int size) {
    if (idx == Set::KNOWN_EMPTY_NODE) {
        return;  // keep our known empty node
    }

    // add to head of free-list
    mem[(int) idx] = freeLists[size];
    freeLists[size] = idx;
}

// tree structure

uint64_t Set::createLeaf(uint8_t* key, int off, int len) {
    uint64_t newNodeRef = allocate(2);
    int a = (int) newNodeRef;
    mem[a++] = ((uint64_t) 1) << key[len - 2];
    mem[a] = ((uint64_t) 1) << key[len - 1];  // value
    len -= 3;
    while (len >= off) {
        uint64_t newParentNodeRef = allocate(2);
        a = (int) newParentNodeRef;
        mem[a++] = ((uint64_t) 1) << key[len--];
        mem[a] = newNodeRef;
        newNodeRef = newParentNodeRef;
    }
    return newNodeRef;
}

uint64_t Set::insertChild(uint64_t nodeRef, uint64_t bitMap, uint64_t bitPos, int idx, uint64_t value) {
    int size = std::popcount(bitMap);
    uint64_t newNodeRef = allocateInsert(nodeRef, size + 1, idx + 1);
    mem[(int) newNodeRef] = bitMap | bitPos;
    mem[(int) newNodeRef + 1 + idx] = value;
    return newNodeRef;
}

uint64_t Set::removeChild(uint64_t nodeRef, uint64_t bitMap, uint64_t bitPos, int idx) {
    int size = std::popcount(bitMap);
    if (size > 1) {
        // node still has other children / leaves
        uint64_t newNodeRef = allocateDelete(nodeRef, size + 1, idx + 1);
        mem[(int) newNodeRef] = bitMap & ~bitPos;
        return newNodeRef;
    } else {
        // node is now empty, remove it
        deallocate(nodeRef, size + 1);
        return Set::KNOWN_DELETED_NODE;
    }
}

// set operations

bool Set::set(uint8_t* key, int len) {
    uint64_t nodeRef = set(root, key, 0, len);
    if (nodeRef != Set::KNOWN_EMPTY_NODE) {
        // denotes change
        count++;
        root = nodeRef;
        return true;
    } else {
        return false;
    }
}

uint64_t Set::set(uint64_t nodeRef, uint8_t* key, int off, int len) {
    uint64_t bitMap = mem[(int) nodeRef];
    uint64_t bitPos = ((uint64_t) 1) << key[off++];  // mind the ++
    int idx = std::popcount(bitMap & (bitPos - 1));

    if ((bitMap & bitPos) == 0) {
        // child not present yet
        uint64_t value;
        if (off == len - 1) {
            value = ((uint64_t) 1) << key[off];
        } else {
            value = createLeaf(key, off, len);
        }
        return insertChild(nodeRef, bitMap, bitPos, idx, value);
    } else {
        // child present
        uint64_t value = mem[(int) nodeRef + 1 + idx];
        if (off == len - 1) {
            // at leaf
            uint64_t bitPosLeaf = ((uint64_t) 1) << key[off];
            if ((value & bitPosLeaf) == 0) {
                // update leaf bitMap
                mem[(int) nodeRef + 1 + idx] = value | bitPosLeaf;
                return nodeRef;
            } else {
                // key already present
                return Set::KNOWN_EMPTY_NODE;
            }
        } else {
            // not at leaf, recursion
            uint64_t childNodeRef = value;
            uint64_t newChildNodeRef = set(childNodeRef, key, off, len);
            if (newChildNodeRef == Set::KNOWN_EMPTY_NODE) {
                return Set::KNOWN_EMPTY_NODE;
            }
            if (newChildNodeRef != childNodeRef) {
                mem[(int) nodeRef + 1 + idx] = newChildNodeRef;
            }
            return nodeRef;
        }
    }
}

bool Set::get(uint8_t* key, int len) {
    if (root == Set::KNOWN_EMPTY_NODE) {
        return false;
    }

    uint64_t nodeRef = root;
    int off = 0;

    for (;;) {
        uint64_t bitMap = mem[(int) nodeRef];
        uint64_t bitPos = ((uint64_t) 1) << key[off++]; // mind the ++
        if ((bitMap & bitPos) == 0) {
            return false; // not found
        }

        uint64_t value = mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];

        if (off == len - 1) {
            // at leaf
            uint64_t bitPosLeaf = ((uint64_t) 1) << key[off];
            return ((value & bitPosLeaf) != 0);
        } else {
            // child pointer
            nodeRef = value;
        }
    }
}

bool Set::predecessor(uint8_t* key, int len) {
    if (root == Set::KNOWN_EMPTY_NODE) {
        return false;
    }

    uint64_t nodeRef = root;
    int off = 0;

    uint64_t nearestNodeRef = Set::KNOWN_EMPTY_NODE;
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

        if (++off == len - 1) {
            // at leaf
            uint64_t bitPosLeaf = ((uint64_t) 1) << key[off];
            // key found
            if ((value & bitPosLeaf) != 0) {
                return true;
            }
            // check if there's a smaller key in the leaf
            if (amt::lowestOneBit(value) < bitPosLeaf) {
                return predecessor(idx, key, off, len);
            }
            // go back to the last node with a bit before the matched bit
            return predecessor(nearestNodeRef, key, nearestOff, len);
        } else {
            // child pointer
            nodeRef = value;
        }
    }
}

bool Set::predecessor(uint64_t nodeRef, uint8_t* key, int off, int len) {
    // no smaller key exists
    if (nodeRef == Set::KNOWN_EMPTY_NODE) {
        return false;
    }

    uint64_t bitMap = mem[(int) nodeRef];
    uint64_t bitPos = ((uint64_t) 1) << key[off];

    // get the largest key that is less than the given key
    uint64_t maskedBitMap = bitMap & (bitPos - 1);
    key[off] = largestKey(maskedBitMap);
    bitPos = ((uint64_t) 1) << key[off++];  // mind the ++

    // nodeRef is a leaf node
    if (off++ == len - 1) {  // mind the ++
        return true;
    }

    // get the largest key in all remaining nodes
    nodeRef = mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
    while (off < len - 1) {
        bitMap = mem[(int) nodeRef];
        key[off] = largestKey(bitMap);
        bitPos = ((uint64_t) 1) << key[off++];  // mind the ++
        nodeRef = mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
    }
    // at leaf
    key[off] = largestKey(nodeRef);

    return true;
}

bool Set::successor(uint8_t* key, int len) {
    if (root == Set::KNOWN_EMPTY_NODE) {
        return false;
    }

    uint64_t nodeRef = root;
    int off = 0;

    uint64_t nearestNodeRef = Set::KNOWN_EMPTY_NODE;
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

        if (++off == len - 1) {
            // at leaf
            uint64_t bitPosLeaf = ((uint64_t) 1) << key[off];
            // key found
            if ((value & bitPosLeaf) != 0) {
                return true;
            }
            // check if there's a larger key in the leaf
            if (amt::highestOneBit(value) > bitPosLeaf) {
                return successor(idx, key, off, len);
            }
            // go back to the last node with a bit after the matched bit
            return successor(nearestNodeRef, key, nearestOff, len);
        } else {
            // child pointer
            nodeRef = value;
        }
    }
}

bool Set::successor(uint64_t nodeRef, uint8_t* key, int off, int len) {
    // no smaller key exists
    if (nodeRef == Set::KNOWN_EMPTY_NODE) {
        return false;
    }

    uint64_t bitMap = mem[(int) nodeRef];
    uint64_t bitPos = ((uint64_t) 1) << (key[off] + 1);  // +1 because bitMap is exclusive

    // get the smallest key that is greater than the given key
    uint64_t maskedBitMap = bitMap & ~(bitPos - 1);
    key[off] = smallestKey(maskedBitMap);
    bitPos = ((uint64_t) 1) << key[off];

    // nodeRef is a leaf node
    if (off++ == len - 1) {  // mind the ++
        return true;
    }

    // get the smallest key in all remaining nodes
    nodeRef = mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
    while (off < len - 1) {
        bitMap = mem[(int) nodeRef];
        key[off] = smallestKey(bitMap);
        bitPos = ((uint64_t) 1) << key[off++];  // mind the ++
        nodeRef = mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
    }
    // at leaf
    key[off] = smallestKey(nodeRef);

    return true;
}

bool Set::clear(uint8_t* key, int len) {
    uint64_t nodeRef = clear(root, key, 0, len);
    if (nodeRef != Set::KNOWN_EMPTY_NODE) {
        count--;
        if (nodeRef == Set::KNOWN_DELETED_NODE) {
            root = Set::KNOWN_EMPTY_NODE;
        } else {
            root = nodeRef;
        }
        return true;
    } else {
        return false;
    }
}

uint64_t Set::clear(uint64_t nodeRef, uint8_t* key, int off, int len) {
    if (root == Set::KNOWN_EMPTY_NODE) {
        return Set::KNOWN_EMPTY_NODE;
    }

    uint64_t bitMap = mem[(int) nodeRef];
    uint64_t bitPos = ((uint64_t) 1) << key[off++];  // mind the ++
    if ((bitMap & bitPos) == 0) {
        // child not present, key not found
        return Set::KNOWN_EMPTY_NODE;
    } else {
        // child present
        int idx = std::popcount(bitMap & (bitPos - 1));
        uint64_t value = mem[(int) nodeRef + 1 + idx];
        if (off == len - 1) {
            // at leaf
            uint64_t bitPosLeaf = ((uint64_t) 1) << key[off];
            if ((value & bitPosLeaf) == 0) {
                // key not present
                return Set::KNOWN_EMPTY_NODE;
            } else {
                // clear bit in leaf
                value = value & ~bitPosLeaf;
                if (value != 0) {
                    // leaf still has some bits set, keep leaf but update
                    mem[(int) nodeRef + 1 + idx] = value;
                    return nodeRef;
                } else {
                    return removeChild(nodeRef, bitMap, bitPosLeaf, idx);
                }
            }
        } else {
            // not at leaf
            uint64_t childNodeRef = value;
            uint64_t newChildNodeRef = clear(childNodeRef, key, off, len);
            if (newChildNodeRef == Set::KNOWN_EMPTY_NODE) {
                return Set::KNOWN_EMPTY_NODE;
            }
            if (newChildNodeRef == Set::KNOWN_DELETED_NODE) {
                return removeChild(nodeRef, bitMap, bitPos, idx);
            }
            if (newChildNodeRef != childNodeRef) {
                mem[(int) nodeRef + 1 + idx] = newChildNodeRef;
            }
            return nodeRef;
        }
    }
}

}
