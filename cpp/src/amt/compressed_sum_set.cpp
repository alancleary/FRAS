#include <algorithm>
#include <bit>
#include "amt/bitops.hpp"
#include "amt/key.hpp"
#include "amt/compressed_sum_set.hpp"

#include <iostream>

namespace amt {

void CompressedSumSet::tmp(int len)
{
    // output values and sums
    uint8_t* key = new uint8_t[len];
    tmp(root, key, 0, len);
    delete[] key;
}

void CompressedSumSet::tmp(uint64_t nodeRef, uint8_t* key, int off, int len)
{
    uint64_t bitMap = mem[(int) nodeRef];
    if (bitMap == 0 && !compressed[(int) nodeRef]) {
        return;
    }
    if (compressed[(int) nodeRef]) {
        std::cerr << "v2 sum : " << mem[(int) nodeRef + 1] << std::endl;
        std::cerr << "v2 ckey (compressed): " << bitMap << std::endl;
        return;
    }
    int numChildren = std::popcount(bitMap);
    if (off == len - 2) {
        uint64_t sum = mem[(int) nodeRef + 1 + numChildren];
        std::cerr << "v2 sum: " << sum << std::endl;
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
                key[off+1] = (uint8_t) bitNum2;
                std::cerr << "v2 ckey: " << get6Int(key) << std::endl;
            }
        } else {
            uint64_t childNode = mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
            tmp(childNode, key, off + 1, len);
        }                
    }
}

// construction

CompressedSumSet::CompressedSumSet(Set& set, int len, GetKey getKey, SetKey setKey): getKey(getKey), setKey(setKey) {
    // initial values
    memSize = 0;
    sumCount = 0;
    root = CompressedSumSet::KNOWN_EMPTY_NODE;
    count = set.size();
    // pre-compute the memory required
    computeCompressedSize(set, len);
    // allocate the memory
    int buffSize = len * 2;  // tails are compressed after they're added
    mem = new uint64_t[memSize + buffSize + sumCount]; 
    compressed = new bool[memSize + sumCount];
    // construct the tree
    construct(set, len);
    std::cerr << "memSize: " << memSize << std::endl;
    std::cerr << "sumCount: " << sumCount << std::endl;
    std::cerr << "total: " << memSize + sumCount << std::endl;
    std::cerr << "memory: " << 64 * (memSize + sumCount) << " bits" << std::endl;
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
    // leaf parents get a partial sum; will be used by leaf if tail compressed
    if (off == len - 2) {
        this->sumCount += 1;
    }
    // iterate the set bits
    uint64_t bits = bitMap;
    while (bits != 0) {
        uint64_t bitPos = bits & -bits; bits ^= bitPos; // get rightmost bit and clear it
        // the bit's nodeRef is a leaf
        if (off == len - 2) {
            uint64_t value = set.mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
            // the leaf and nodeRef are the start of a compressible tail
            if (isTail && std::popcount(value) == 1) {
                return 2;  // bitMap + value (leaf) = 2 non-branching nodes
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
                this->memSize += 2;  // tail compressed node and nodeRef to it
            }
        }
    }
    // bitMap is not tail compressible
    this->memSize += 2;  // nodeRef + bitMap
    return 0;
}

void CompressedSumSet::construct(Set& set, int len)
{
    uint8_t* key = new uint8_t[len];
    int idx = 1;  // idx starts at 1 because 0 is KNOWN_EMPTY_NODE
    root = idx;
    int sum = 0;
    int tailLen = construct(set, len, set.root, key, 0, idx, sum);
    // edge case: root is a tail, i.e. there's only one value in the set
    if (tailLen > 0) {
        mem[root] = getKey(key);
        compressed[root] = true;
        mem[root + 1] = sum;  // should be 0
    // edge case: the set is empty
    } else if (idx == 1) {
        root = CompressedSumSet::KNOWN_EMPTY_NODE;
    }
    delete[] key;
}

int CompressedSumSet::construct(Set& set, int len, uint64_t nodeRef, uint8_t* key, int off, int& idx, int& sum) {
    uint64_t bitMap = set.mem[(int) nodeRef];
    // prepare the mem indexes
    //int nodeIdx = idx;
    int childIdx = idx + 1;
    int numChildren = std::popcount(bitMap);
    bool isTail = numChildren == 1;  // there's only one bit set
    // assume the node is not tail compressed
    mem[idx] = bitMap;
    compressed[idx] = false;
    // space for children (leafs) and partial sum
    if (off == len - 2) {
        // save the partial sum before processing the leafs
        mem[idx + 1 + numChildren] = sum;
        compressed[idx + 1 + numChildren] = false;
        idx += numChildren + 2;
    // space for children (not leafs) only
    } else {
        idx += numChildren + 1;
    }
    // iterate the set bits
    uint64_t bits = bitMap;
    while (bits != 0) {
        uint64_t bitPos = bits & -bits; bits ^= bitPos; // get rightmost bit and clear it
        int bitNum = std::countr_zero(bitPos);
        key[off] = (uint8_t) bitNum;
        // the bit's nodeRef is a leaf
        if (off == len - 2) {
            uint64_t value = set.mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
            int leafValues = std::popcount(value);
            // the leaf and nodeRef are the start of a compressible tail
            if (isTail && leafValues == 1) {
                uint64_t bitPos2 = value & -value;
                int bitNum2 = std::countr_zero(bitPos2);
                key[off + 1] = (uint8_t) bitNum2;
                return 2;  // leaf + bitMap = 2 non-branching nodes
            }
            // add the leaf as the child
            mem[childIdx] = value;
            compressed[childIdx] = false;
            // add the leaf's count to the sum
            sum += leafValues;
        } else {
            // add a pointer to the child index
            mem[childIdx] = idx;
            // construct the subtree rooted at the child
            uint64_t childNode = set.mem[(int) nodeRef + 1 + std::popcount(bitMap & (bitPos - 1))];
            int tailLen = construct(set, len, childNode, key, off + 1, idx, sum);
            // the descendants and bitMap are a compressible tail
            if (isTail && tailLen > 0) {
                return tailLen + 1;
            // the descendants are a compressible tail but bitMap has multiple children
            } else if (tailLen > 0) {
                // add the tail as the child
                mem[mem[childIdx]] = getKey(key);
                compressed[mem[childIdx]] = true;
                // add the sum for the tail
                mem[mem[childIdx] + 1] = sum;
                compressed[mem[childIdx] + 1] = false;
                // add the tail's count to the sum
                sum += 1;
                // adjust the index because the tail is compressed
                // idx - ((2 * tailLen) - 1) + 2
                // idx: currently at the index after the non-compressed tail
                // (2 * tailLen) - 1: the nodes and nodeRefs of the tail; -1 because the last node doesn't have a nodeRef
                // + 2: a node to store the tail-compressed value and a nodeRef to points to it
                idx = idx - (2 * tailLen) + 3;
            }
        }
        childIdx++;
    }
    // nodeRef is not tail compressible
    //mem[nodeIdx] = bitMap;
    //compressed[nodeIdx] = false;
    return 0;
}

// destruction

CompressedSumSet::~CompressedSumSet() {
    delete[] mem;
    delete[] compressed;
}

// set operations

/*
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
*/

uint64_t CompressedSumSet::predecessor(uint8_t* key, int len) {
    if (root == CompressedSumSet::KNOWN_EMPTY_NODE) {
        throw std::runtime_error("No key to select");
    }

    uint64_t keyValue = getKey(key);
    uint64_t nodeRef = root;
    int off = 0;

    uint64_t nearestNodeRef = CompressedSumSet::KNOWN_EMPTY_NODE;
    int nearestOff = 0;

    for (;;) {
        // get the next node
        uint64_t bitMap = mem[(int) nodeRef];

        // check if the node is tail compressed
        if (compressed[(int) nodeRef]) {
            if (bitMap <= keyValue) {
                setKey(key, bitMap);
                return mem[(int) nodeRef + 1];
            }
            return predecessor(nearestNodeRef, key, nearestOff, len);
        }

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

        // get the next nodeRef/leaf
        uint64_t idx = nodeRef + 1 + std::popcount(bitMap & (bitPos - 1));
        uint64_t value = mem[(int) idx];

        // value is a leaf
        if (++off == len - 1) {
            // at leaf
            uint64_t bitPosLeaf = ((uint64_t) 1) << key[off];
            // key found
            if ((value & bitPosLeaf) != 0) {
                int numChildren = std::popcount(bitMap);
                uint64_t maskedLeaf = value & (bitPosLeaf - 1);
                //int child = std::popcount(maskedLeaf);
                //return mem[(int) idx + numChildren] + child;
                // compute the partial sum
                //int numChildren = std::popcount(bitMap);
                uint64_t sum = mem[(int) nodeRef + 1 + numChildren];
                for (nodeRef = nodeRef + 1; nodeRef < idx; nodeRef++) {
                    sum += std::popcount(mem[(int) nodeRef]);
                }
                return sum + std::popcount(maskedLeaf);
            }
            // check if there's a smaller key in the leaf
            if (amt::lowestOneBit(value) < bitPosLeaf) {
                // NOTE: the return value is a bitMap instead of a partial sum
                uint64_t maskedLeaf = predecessor(idx, key, off, len);
                //int numChildren = std::popcount(bitMap);
                //int child = std::popcount(maskedLeaf);
                //return mem[(int) idx + numChildren] + child - 1;
                // compute the partial sum
                int numChildren = std::popcount(bitMap);
                uint64_t sum = mem[(int) nodeRef + 1 + numChildren];
                for (nodeRef = nodeRef + 1; nodeRef < idx; nodeRef++) {
                    sum += std::popcount(mem[(int) nodeRef]);
                }
                return sum + std::popcount(maskedLeaf);
            }
            // go back to the last node with a bit before the matched bit
            return predecessor(nearestNodeRef, key, nearestOff, len);
        // value is a nodeRef
        } else {
            nodeRef = value;
        }
    }
}

uint64_t CompressedSumSet::predecessor(uint64_t nodeRef, uint8_t* key, int off, int len) {
    // no smaller key exists
    if (nodeRef == CompressedSumSet::KNOWN_EMPTY_NODE) {
        throw std::runtime_error("No key to select");
    }

    uint64_t bitMap = mem[(int) nodeRef];
    uint64_t bitPos = ((uint64_t) 1) << key[off];

    // get the largest key that is less than the given key
    uint64_t maskedBitMap = bitMap & (bitPos - 1);
    key[off] = largestKey(maskedBitMap);

    // nodeRef is a leaf node
    if (off == len - 1) {
        // return the maskedBitMap so the caller can compute the partial sum
        return maskedBitMap;
    }

    // get the largest key in all remaining nodes
    bitPos = ((uint64_t) 1) << key[off++];  // mind the ++
    uint64_t idx = nodeRef + 1 + std::popcount(bitMap & (bitPos - 1));
    uint64_t nextNodeRef = mem[(int) idx];
    while (off < len - 1) {
        bitMap = mem[(int) nextNodeRef];

        // check if the node is tail compressed
        if (compressed[(int) nextNodeRef]) {
            setKey(key, bitMap);
            return mem[(int) nextNodeRef + 1];
        }

        key[off] = largestKey(bitMap);
        bitPos = ((uint64_t) 1) << key[off++];  // mind the ++
        idx = nextNodeRef + 1 + std::popcount(bitMap & (bitPos - 1));
        nodeRef = nextNodeRef;
        nextNodeRef = mem[(int) idx];
    }
    // at leaf
    key[off] = largestKey(nextNodeRef);
    // compute the partial sum
    int numChildren = std::popcount(bitMap);
    uint64_t sum = mem[(int) nodeRef + 1 + numChildren];
    for (nodeRef = nodeRef + 1; nodeRef <= idx; nodeRef++) {
        sum += std::popcount(mem[(int) nodeRef]);
    }
    return sum;
    //int child = std::popcount(nextNodeRef);
    //return mem[(int) idx + numChildren] + child - 1;
}

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
