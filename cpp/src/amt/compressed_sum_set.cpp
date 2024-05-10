#include <algorithm>
#include <bit>
#include "cfg-amt/amt/bitops.hpp"
#include "cfg-amt/amt/key.hpp"
#include "cfg-amt/amt/compressed_sum_set.hpp"

#include <iostream>

namespace cfg_amt {

void CompressedSumSet::tmp(Set& set, uint64_t nodeRef, uint8_t* key, int off, int len)
{
    uint64_t bitMap = set.mem[(int) nodeRef];
    if (bitMap == 0) {
        return;
    }
    uint64_t bits = bitMap;
    while (bits != 0) {
        uint64_t bitPos = bits & -bits; bits ^= bitPos; // get rightmost bit and clear it
        int bitNum = std::countr_zero(bitPos);
        key[off] = (uint8_t) bitNum;
        if (off == len - 2) {
            uint64_t value = set.mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
            uint64_t bits2 = value;
            while (bits2 != 0) {
                uint64_t bitPos2 = bits2 & -bits2; bits2 ^= bitPos2;
                int bitNum2 = std::countr_zero(bitPos2);
                key[off+1] = (uint8_t) bitNum2;
                std::cerr << "set key: " << get6Int(key) << std::endl;
            }
        } else {
            uint64_t childNode = set.mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
            tmp(set, childNode, key, off + 1, len);
        }                
    }
}

// construction

CompressedSumSet::CompressedSumSet(Set& set, int len, GetKey getKey): getKey(getKey) {
    uint8_t* key = new uint8_t[6];
    this->tmp(set, set.root, key, 0, 6);
    // initial values
    memSize = 0;
    root = CompressedSumSet::KNOWN_EMPTY_NODE;
    // pre-compute the memory required
    computeCompressedSize(set, len);
    std::cerr << "num values: " << set.size() << std::endl;
    std::cerr << "set memSize: " << set.freeIdx << std::endl;
    std::cerr << "tail memSize: " << memSize << std::endl;
    // allocate the memory
    int buffSize = (len * 2) - 1;  // tails are compressed after they're added
    mem = new uint64_t[memSize + buffSize + leafCount]; 
    compressed = new bool[memSize + len];
    // construct the tree
    construct(set, len);
}

void CompressedSumSet::computeCompressedSize(Set& set, int len)
{
    int tailLen = computeCompressedSize(set, len, set.root, 0);
    // edge case: root is a tail, i.e. there's only one value in the set
    if (tailLen > 0) {
        this->memSize += 1;
    // there's no pointer to the root node
    } else {
        this->memSize -= 1;
    }
}

int CompressedSumSet::computeCompressedSize(Set& set, int len, uint64_t nodeRef, int off) {
    uint64_t bitMap = set.mem[(int) nodeRef];
    if (bitMap == 0) {
        return 0;
    }
    bool isTail = std::popcount(bitMap) == 1;  // there's only one bit set
    // iterate the set bits
    uint64_t bits = bitMap;
    while (bits != 0) {
        uint64_t bitPos = bits & -bits; bits ^= bitPos; // get rightmost bit and clear it
        // the bit's nodeRef is a leaf
        if (off == len - 2) {
            this->leafCount += 1;
            uint64_t value = set.mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
            // the leaf and nodeRef are the start of a compressible tail
            if (isTail && std::popcount(value) == 1) {
                return 2;  // leaf + nodeRef = 2 non-branching nodes
            }
            this->memSize += 1;
        } else {
            uint64_t childNode = set.mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
            int tailLen = computeCompressedSize(set, len, childNode, off + 1);
            // the descendants and nodeRef are a compressible tail
            if (isTail && tailLen > 0) {
                return tailLen + 1;
            // the descendants are a compressible tail
            } else if (tailLen > 0) {
                this->memSize += 1;  // entire tail is compressed into first pointer of tail
            }
        }
    }
    // nodeRef is not tail compressible
    this->memSize += 2;  // pointer to nodeRef + nodeRef
    return 0;
}

void CompressedSumSet::construct(Set& set, int len)
{
    uint8_t* key = new uint8_t[len];
    int idx = 0;
    int sum = 0;
    int tailLen = construct(set, len, set.root, key, 0, idx, sum);
    // edge case: root is a tail, i.e. there's only one value in the set
    if (tailLen > 0) {
        mem[0] = getKey(key);
        compressed[0] = true;
    }
    delete[] key;
}

int CompressedSumSet::construct(Set& set, int len, uint64_t nodeRef, uint8_t* key, int off, int& idx, int& sum) {
    uint64_t bitMap = set.mem[(int) nodeRef];
    // prepare the mem indexes
    int nodeIdx = idx;
    int childIdx = idx + 1;
    int numChildren = std::popcount(bitMap);
    idx += numChildren + 1;
    bool isTail = numChildren == 1;  // there's only one bit set
    // iterate the set bits
    uint64_t bits = bitMap;
    while (bits != 0) {
        uint64_t bitPos = bits & -bits; bits ^= bitPos; // get rightmost bit and clear it
        int bitNum = std::countr_zero(bitPos);
        key[off] = (uint8_t) bitNum;
        // the bit's node is a leaf
        if (off == len - 2) {
            uint64_t value = set.mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
            // the leaf and nodeRef are the start of a compressible tail
            if (isTail && std::popcount(value) == 1) {
                return 2;  // leaf + nodeRef = 2 non-branching nodes
            }
            // add the leaf as the child
            mem[childIdx] = value;
            compressed[childIdx] = false;
        } else {
            // add a pointer to the child index
            mem[childIdx] = idx;
            // construct the subtree rooted at the child
            uint64_t childNode = set.mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
            int tailLen = construct(set, len, childNode, key, off + 1, idx, sum);
            // the descendants and nodeRef are a compressible tail
            if (isTail && tailLen > 0) {
                return tailLen + 1;
            // the descendants are a compressible tail
            } else if (tailLen > 0) {
                mem[childIdx] = getKey(key);
                compressed[childIdx] = true;
                idx = idx - (2* tailLen) + 2;
            }
        }
    }
    // nodeRef is not tail compressible
    mem[nodeIdx] = nodeRef;
    compressed[nodeIdx] = false;
    return 0;
}

// destruction

CompressedSumSet::~CompressedSumSet() {
    delete[] mem;
    delete[] compressed;
}

// set operations

bool CompressedSumSet::get(uint8_t* key, int len) {
    if (root == CompressedSumSet::KNOWN_EMPTY_NODE) {
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

/*
bool CompressedSumSet::predecessor(uint8_t* key, int len) {
    if (root == CompressedSumSet::KNOWN_EMPTY_NODE) {
        return false;
    }

    uint64_t nodeRef = root;
    int off = 0;

    uint64_t nearestNodeRef = CompressedSumSet::KNOWN_EMPTY_NODE;
    int nearestOff = 0;

    for (;;) {
        uint64_t bitMap = mem[(int) nodeRef];
        uint64_t bitPos = ((uint64_t) 1) << key[off];

        // memoize the node if it has smaller keys
        if (cfg_amt::lowestOneBit(bitMap) < bitPos) {
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
            if (cfg_amt::lowestOneBit(value) < bitPosLeaf) {
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
*/

/*
bool CompressedSumSet::predecessor(uint64_t nodeRef, uint8_t* key, int off, int len) {
    // no smaller key exists
    if (nodeRef == CompressedSumSet::KNOWN_EMPTY_NODE) {
        return false;
    }

    uint64_t bitMap = mem[(int) nodeRef];
    uint64_t bitPos = ((uint64_t) 1) << key[off];

    // get the largest key that is less than the given key
    uint64_t maskedBitMap = bitMap & (bitPos - 1);
    key[off] = largestKey(maskedBitMap);
    bitPos = ((uint64_t) 1) << key[off];

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
*/

/*
bool CompressedSumSet::successor(uint8_t* key, int len) {
    if (root == CompressedSumSet::KNOWN_EMPTY_NODE) {
        return false;
    }

    uint64_t nodeRef = root;
    int off = 0;

    uint64_t nearestNodeRef = CompressedSumSet::KNOWN_EMPTY_NODE;
    int nearestOff = 0;

    for (;;) {
        uint64_t bitMap = mem[(int) nodeRef];
        uint64_t bitPos = ((uint64_t) 1) << key[off];

        // memoize the node if it has larger keys
        if (cfg_amt::highestOneBit(bitMap) > bitPos) {
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
            if (cfg_amt::highestOneBit(value) > bitPosLeaf) {
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
*/

/*
bool CompressedSumSet::successor(uint64_t nodeRef, uint8_t* key, int off, int len) {
    // no smaller key exists
    if (nodeRef == CompressedSumSet::KNOWN_EMPTY_NODE) {
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
*/

}
