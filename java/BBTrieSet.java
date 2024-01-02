import java.util.Arrays;


/** A set implemented as a bitwise trie with a bitmap. */
public class BBTrieSet {

    long[] mem;
    long[] freeLists;
    long freeIdx;

    long root;
    long count;

    // maximum node size is 1 (bitMap) + 64 (child pointers or leaf values) + 1 as arrays are zero based
    final static int FREE_LIST_SIZE = 1+64+1;

    final static int KNOWN_EMPTY_NODE = 0;
    final static int KNOWN_DELETED_NODE = 1;
    final static int HEADER_SIZE = 2; // KNOWN_EMPTY_NODE, KNOWN_DELETED_NODE

    public BBTrieSet(int size) {
        mem = new long[size];
        freeLists = new long[FREE_LIST_SIZE];
        freeIdx = HEADER_SIZE;
        root = KNOWN_EMPTY_NODE;
        count = 0;
    }

    private long allocate(int size) {
        long free = freeLists[size];
        if (free != 0) {
            // requested size available in free list, re-link and return head
            freeLists[size] = mem[(int) free];
            return free;
        } else {
            // expansion required?
            if (freeIdx + size > mem.length) {
                // increase by 25% and assure this is enough
                int currSize = mem.length;
                int newSize = currSize + Math.max(currSize / 4, size);
                mem = Arrays.copyOf(mem, newSize);
            }
            long idx = freeIdx;
            freeIdx += size;
            return idx;
        }
    }

    private long allocateInsert(long nodeIdx, int size, int childIdx) {
        long newNodeRef = allocate(size + 1);

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

    private long allocateDelete(long nodeIdx, int size, int childIdx) {
        long newNodeRef = allocate(size - 1);

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

    private void deallocate(long idx, int size) {
        if (idx == KNOWN_EMPTY_NODE) {
            return; // keep our known empty node
        }

        // add to head of free-list
        mem[(int) idx] = freeLists[size];
        freeLists[size] = idx;
    }

    private long createLeaf(byte[] key, int off, int len) {
        long newNodeRef = allocate(2);
        int a = (int) newNodeRef;
        mem[a++] = 1L << key[len - 2];
        mem[a] = 1L << key[len - 1]; // value
        len -= 3;
        while (len >= off) {
            long newParentNodeRef = allocate(2);
            a = (int) newParentNodeRef;
            mem[a++] = 1L << key[len--];
            mem[a] = newNodeRef;
            newNodeRef = newParentNodeRef;
        }
        return newNodeRef;
    }

    private long insertChild(long nodeRef, long bitMap, long bitPos, int idx, long value) {
        int size = Long.bitCount(bitMap);
        long newNodeRef = allocateInsert(nodeRef, size + 1, idx + 1);
        mem[(int) newNodeRef] = bitMap | bitPos;
        mem[(int) newNodeRef + 1 + idx] = value;
        return newNodeRef;
    }

    private long removeChild(long nodeRef, long bitMap, long bitPos, int idx) {
        int size = Long.bitCount(bitMap);
        if (size > 1) {
            // node still has other children / leaves
            long newNodeRef = allocateDelete(nodeRef, size + 1, idx + 1);
            mem[(int) newNodeRef] = bitMap & ~bitPos;
            return newNodeRef;
        } else {
            // node is now empty, remove it
            deallocate(nodeRef, size + 1);
            return KNOWN_DELETED_NODE;
        }
    }

    public long size() {
        return count;
    }

    /**
      * Checks if the given byte key exists in the set.
      *
      * @param key The byte key.
      * @param len The length of the byte key.
      * @return A boolean saying whether or not the key exists.
      */
    public boolean get(byte[] key, int len) {
        if (root == KNOWN_EMPTY_NODE) {
            return false;
        }

        long nodeRef = root;
        int off = 0;

        for (;;) {
            long bitMap = mem[(int) nodeRef];
            long bitPos = 1L << key[off++]; // mind the ++
            if ((bitMap & bitPos) == 0) {
                return false; // not found
            }

            long value = mem[(int) nodeRef + 1 + Long.bitCount(bitMap & (bitPos - 1))];

            if (off == len - 1) {
                // at leaf
                long bitPosLeaf = 1L << key[off];
                return ((value & bitPosLeaf) != 0);
            } else {
                // child pointer
                nodeRef = value;
            }
        }
    }

    /**
      * Gets the largest key that is less than or equal to the given byte key. This is equivalent
      * to a paired rank-select query on a bit vector, i.e. select(rank(key)).
      *
      * @param key The byte key to match that will be updated if a different key is selected.
      * @param len The length of the byte key.
      * @return A boolean saying whether or not a key was selected.
      */
    public boolean predecessor(byte[] key, int len) {
        if (root == KNOWN_EMPTY_NODE) {
            return false;
        }

        long nodeRef = root;
        int off = 0;

        long nearestNodeRef = KNOWN_EMPTY_NODE;
        int nearestOff = 0;

        for (;;) {
            long bitMap = mem[(int) nodeRef];
            long bitPos = 1L << key[off];

            // memoize the node if it has smaller keys
            if (Long.compareUnsigned(Long.lowestOneBit(bitMap), bitPos) < 0) {
                nearestNodeRef = nodeRef;
                nearestOff = off;
            }

            // key not found
            if ((bitMap & bitPos) == 0) {
                return predecessor(nearestNodeRef, key, nearestOff, len);
            }

            long idx = nodeRef + 1 + Long.bitCount(bitMap & (bitPos - 1));
            long value = mem[(int) idx];

            if (++off == len - 1) {
                // at leaf
                long bitPosLeaf = 1L << key[off];
                // key found
                if ((value & bitPosLeaf) != 0) {
                    return true;
                }
                // check if there's a smaller key in the leaf
                if (Long.compareUnsigned(Long.lowestOneBit(value), bitPosLeaf) < 0) {
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

    /**
      * Selects the largest key that is less than the key predecessor failed to match. Assumes that
      * the given nodeRef has at least one key that is less than the given byte key or that it is
      * the KNOWN_EMPTY_NODE value.
      *
      * @param nodeRef The node to start the selection from.
      * @param key The partially matched byte key that will be updated if a different key is selected.
      * @param off The offset into the key that corresponds to the nodeRef node.
      * @param len The length of the byte key.
      * @return A boolean saying whether or not a key was selected.
      */
    private boolean predecessor(long nodeRef, byte[] key, int off, int len) {
        // no smaller key exists
        if (nodeRef == KNOWN_EMPTY_NODE) {
            return false;
        }

        long bitMap = mem[(int) nodeRef];
        long bitPos = 1L << key[off];

        // get the largest key that is less than the given key
        long maskedBitMap = bitMap & (bitPos - 1);
        key[off] = largestKey(maskedBitMap);
        bitPos = 1L << key[off];

        // nodeRef is a leaf node
        if (off++ == len - 1) {  // mind the ++
            return true;
        }

        // get the largest key in all remaining nodes
        nodeRef = mem[(int) nodeRef + 1 + Long.bitCount(bitMap & (bitPos - 1))];
        while (off < len - 1) {
            bitMap = mem[(int) nodeRef];
            key[off] = largestKey(bitMap);
            bitPos = 1L << key[off++];  // mind the ++
            nodeRef = mem[(int) nodeRef + 1 + Long.bitCount(bitMap & (bitPos - 1))];
        }
        // at leaf
        key[off] = largestKey(nodeRef);

        return true;
    }

    /**
      * Computes the largest key in the given bitmap.
      *
      * @param bitmap The bitmap to compute the largest key in.
      * @return The largest key.
      */
    private byte largestKey(long bitMap) {
        return (byte) (Long.SIZE - Long.numberOfLeadingZeros(bitMap) - 1);
    }

    /**
      * Gets the smallest key that is greater than or equal to the given byte key. This is equivalent
      * to a paired rank-select query on a bit vector, i.e. select(rank(key) + 1).
      *
      * @param key The byte key to match that will be updated if a different key is selected.
      * @param len The length of the byte key.
      * @return A boolean saying whether or not a key was selected.
      */
    public boolean successor(byte[] key, int len) {
        if (root == KNOWN_EMPTY_NODE) {
            return false;
        }

        long nodeRef = root;
        int off = 0;

        long nearestNodeRef = KNOWN_EMPTY_NODE;
        int nearestOff = 0;

        for (;;) {
            long bitMap = mem[(int) nodeRef];
            long bitPos = 1L << key[off];

            // memoize the node if it has larger keys
            if (Long.compareUnsigned(Long.highestOneBit(bitMap), bitPos) > 0) {
                nearestNodeRef = nodeRef;
                nearestOff = off;
            }

            // key not found
            if ((bitMap & bitPos) == 0) {
                return successor(nearestNodeRef, key, nearestOff, len);
            }

            long idx = nodeRef + 1 + Long.bitCount(bitMap & (bitPos - 1));
            long value = mem[(int) idx];

            if (++off == len - 1) {
                // at leaf
                long bitPosLeaf = 1L << key[off];
                // key found
                if ((value & bitPosLeaf) != 0) {
                    return true;
                }
                // check if there's a larger key in the leaf
                if (Long.compareUnsigned(Long.highestOneBit(value), bitPosLeaf) > 0) {
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

    /**
      * Selects the smallest key that is larger than the key successor failed to match. Assumes that
      * the given nodeRef has at least one key that is greater than the given byte key or that it is
      * the KNOWN_EMPTY_NODE value.
      *
      * @param nodeRef The node to start the selection from.
      * @param key The partially matched byte key that will be updated if a different key is selected.
      * @param off The offset into the key that corresponds to the nodeRef node.
      * @param len The length of the byte key.
      * @return A boolean saying whether or not a key was selected.
      */
    private boolean successor(long nodeRef, byte[] key, int off, int len) {
        // no smaller key exists
        if (nodeRef == KNOWN_EMPTY_NODE) {
            return false;
        }

        long bitMap = mem[(int) nodeRef];
        long bitPos = 1L << (key[off] + 1);  // +1 because bitMap is exclusive

        // get the smallest key that is greater than the given key
        long maskedBitMap = bitMap & ~(bitPos - 1);
        key[off] = smallestKey(maskedBitMap);
        bitPos = 1L << key[off];

        // nodeRef is a leaf node
        if (off++ == len - 1) {  // mind the ++
            return true;
        }

        // get the smallest key in all remaining nodes
        nodeRef = mem[(int) nodeRef + 1 + Long.bitCount(bitMap & (bitPos - 1))];
        while (off < len - 1) {
            bitMap = mem[(int) nodeRef];
            key[off] = smallestKey(bitMap);
            bitPos = 1L << key[off++];  // mind the ++
            nodeRef = mem[(int) nodeRef + 1 + Long.bitCount(bitMap & (bitPos - 1))];
        }
        // at leaf
        key[off] = smallestKey(nodeRef);

        return true;
    }

    /**
      * Computes the smallest key in the given bitmap.
      *
      * @param bitmap The bitmap to compute the smallest key in.
      * @return The smallest key.
      */
    private byte smallestKey(long bitMap) {
        return (byte) Long.numberOfTrailingZeros(bitMap);
    }

    /**
      * Adds the given byte key to the set.
      *
      * @param key The byte key.
      * @param len The length of the byte key.
      * @return A boolean saying whether or not the key was added.
      */
    public boolean set(byte[] key, int len) {
        long nodeRef = set(root, key, 0, len);
        if (nodeRef != KNOWN_EMPTY_NODE) {
            // denotes change
            count++;
            root = nodeRef;
            return true;
        } else {
            return false;
        }
    }

    private long set(long nodeRef, byte[] key, int off, int len) {
        long bitMap = mem[(int) nodeRef];
        long bitPos = 1L << key[off++]; // mind the ++
        int idx = Long.bitCount(bitMap & (bitPos - 1));

        if ((bitMap & bitPos) == 0) {
            // child not present yet
            long value;
            if (off == len - 1) {
                value = 1L << key[off];
            } else {
                value = createLeaf(key, off, len);
            }
            return insertChild(nodeRef, bitMap, bitPos, idx, value);
        } else {
            // child present
            long value = mem[(int) nodeRef + 1 + idx];
            if (off == len - 1) {
                // at leaf
                long bitPosLeaf = 1L << key[off];
                if ((value & bitPosLeaf) == 0) {
                    // update leaf bitMap
                    mem[(int) nodeRef + 1 + idx] = value | bitPosLeaf;
                    return nodeRef;
                } else {
                    // key already present
                    return KNOWN_EMPTY_NODE;
                }
            } else {
                // not at leaf, recursion
                long childNodeRef = value;
                long newChildNodeRef = set(childNodeRef, key, off, len);
                if (newChildNodeRef == KNOWN_EMPTY_NODE) {
                    return KNOWN_EMPTY_NODE;
                }
                if (newChildNodeRef != childNodeRef) {
                    mem[(int) nodeRef + 1 + idx] = newChildNodeRef;
                }
                return nodeRef;
            }
        }
    }

    /**
      * Removes the given byte key from the set.
      *
      * @param key The byte key.
      * @param len The length of the byte key.
      * @return A boolean saying whether or not the key was removed.
      */
    public boolean clear(byte[] key, int len) {
        long nodeRef = clear(root, key, 0, len);
        if (nodeRef != KNOWN_EMPTY_NODE) {
            count--;
            if (nodeRef == KNOWN_DELETED_NODE) {
                root = KNOWN_EMPTY_NODE;
            } else {
                root = nodeRef;
            }
            return true;
        } else {
            return false;
        }
    }

    public long clear(long nodeRef, byte[] key, int off, int len) {
        if (root == KNOWN_EMPTY_NODE) {
            return KNOWN_EMPTY_NODE;
        }

        long bitMap = mem[(int) nodeRef];
        long bitPos = 1L << key[off++]; // mind the ++
        if ((bitMap & bitPos) == 0) {
            // child not present, key not found
            return KNOWN_EMPTY_NODE;
        } else {
            // child present
            int idx = Long.bitCount(bitMap & (bitPos - 1));
            long value = mem[(int) nodeRef + 1 + idx];
            if (off == len - 1) {
                // at leaf
                long bitPosLeaf = 1L << key[off];
                if ((value & bitPosLeaf) == 0) {
                    // key not present
                    return KNOWN_EMPTY_NODE;
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
                long childNodeRef = value;
                long newChildNodeRef = clear(childNodeRef, key, off, len);
                if (newChildNodeRef == KNOWN_EMPTY_NODE) {
                    return KNOWN_EMPTY_NODE;
                }
                if (newChildNodeRef == KNOWN_DELETED_NODE) {
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
