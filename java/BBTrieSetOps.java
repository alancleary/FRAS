/** A class containing BBTrieSetset operators and helper methods/classes. */
public class BBTrieSetOps {

    /** An interface that represents physical nodes and "virtual" nodes that are the result of an operator. */
    public interface BBTrieNode {
        public long getBitMap();
        public long getBitMapLeaf(long bitPos);
        public BBTrieNode getChildNode(long bitPos);
    }

    /** Implements the BBTrieNode class that should be used as input to BBTrieNode operator classes. */
    public static class BBTrieNodeMem implements BBTrieNode {

        long nodeRef;
        long[] mem;

        BBTrieNodeMem child;

        public BBTrieNodeMem(long nodeRef, long[] mem) {
            this.nodeRef = nodeRef;
            this.mem = mem;
        }

        @Override
        public long getBitMap() {
            return mem[(int) nodeRef];
        }

        @Override
        public long getBitMapLeaf(long bitPos) {
            int idx = Long.bitCount(getBitMap() & (bitPos - 1));
            long value = mem[(int) nodeRef + 1 + idx];
            return value;
        }

        @Override
        public BBTrieNode getChildNode(long bitPos) {
            int idx = Long.bitCount(getBitMap() & (bitPos - 1));
            long value = mem[(int) nodeRef + 1 + idx];
            return new BBTrieNodeMem(value, mem);
        }

    }

    /** The BBTrieNode representing a set intersection. */
    public static class BBTrieAnd implements BBTrieNode {

        BBTrieNode nodeA;
        BBTrieNode nodeB;
        long bitMapA;
        long bitMapB;

        public BBTrieAnd(BBTrieNode nodeA, BBTrieNode nodeB) {
            this.nodeA = nodeA;
            this.nodeB = nodeB;
            bitMapA = nodeA.getBitMap();
            bitMapB = nodeB.getBitMap();
        }

        public long getBitMap() {
            return bitMapA & bitMapB; // this gives a nice optimization (pruning)
        }

        public long getBitMapLeaf(long bitPos) {
            return nodeA.getBitMapLeaf(bitPos) & nodeB.getBitMapLeaf(bitPos);
        }

        public BBTrieNode getChildNode(long bitPos) {
            BBTrieNode childNodeA = nodeA.getChildNode(bitPos);
            BBTrieNode childNodeB = nodeB.getChildNode(bitPos);
            return new BBTrieAnd(childNodeA, childNodeB);
        }

    }

    /** The BBTrieNode representing a set union. */
    public static class BBTrieOr implements BBTrieNode {

        BBTrieNode nodeA;
        BBTrieNode nodeB;
        long bitMapA;
        long bitMapB;

        public BBTrieOr(BBTrieNode nodeA, BBTrieNode nodeB) {
            this.nodeA = nodeA;
            this.nodeB = nodeB;
            bitMapA = nodeA.getBitMap();
            bitMapB = nodeB.getBitMap();
        }

        public long getBitMap() {
            return bitMapA | bitMapB;
        }

        public long getBitMapLeaf(long bitPos) {
            return nodeA.getBitMapLeaf(bitPos) | nodeB.getBitMapLeaf(bitPos);
        }

        public BBTrieNode getChildNode(long bitPos) {
            if ((bitMapA & bitPos) != 0) {
                BBTrieNode childNodeA = nodeA.getChildNode(bitPos);
                if ((bitMapB & bitPos) != 0) {
                    BBTrieNode childNodeB = nodeB.getChildNode(bitPos);
                    return new BBTrieOr(childNodeA, childNodeB);
                } else {
                    return childNodeA; // optimization, no more or-node required
                }
            } else {
                BBTrieNode childNodeB = nodeB.getChildNode(bitPos);
                return childNodeB; // optimization, no more or-node required
            }
        }

    }

    /** The BBTrieNode representing a set difference. */
    public static class BBTrieMinus implements BBTrieNode {

        BBTrieNode nodeA;
        BBTrieNode nodeB;
        long bitMapA;
        long bitMapB;

        public BBTrieMinus(BBTrieNode nodeA, BBTrieNode nodeB) {
            this.nodeA = nodeA;
            this.nodeB = nodeB;
            bitMapA = nodeA.getBitMap();
            bitMapB = nodeB.getBitMap();
        }

        public long getBitMap() {
            return bitMapA; // bitMapB not useful here
        }

        public long getBitMapLeaf(long bitPos) {
            long childBitMapA = nodeA.getBitMapLeaf(bitPos);
            if ((bitMapB & bitPos) == 0) {
                return childBitMapA;
            }
            long childBitMapB = nodeB.getBitMapLeaf(bitPos);
            return childBitMapA & ~childBitMapB;
        }

        public BBTrieNode getChildNode(long bitPos) {
            BBTrieNode childNodeA = nodeA.getChildNode(bitPos);
            if ((bitMapB & bitPos) == 0) {
                return childNodeA; // optimization, no more minus-node required
            }
            BBTrieNode childNodeB = nodeB.getChildNode(bitPos);
            return new BBTrieMinus(childNodeA, childNodeB);
        }

    }

    /** The BBTrieNode representing a range query. */
    public static class BBTrieIntRange implements BBTrieNode {

        private long bitMap;
        private int a, b;
        private int x, y;
        private int level;

        public BBTrieIntRange(int a, int b) {
            this(a, b, 5);
        }

        private BBTrieIntRange (int a, int b, int level) {
            this.a = a;
            this.b = b;
            this.level = level;
            x = (int) (a >>> (level * 6)) & 0x3F;
            y = (int) (b >>> (level * 6)) & 0x3F;

            // bit hack for: for (int i = x; i <= y; i++) bitSet |= (1L << i);
            bitMap = 1L << y;
            bitMap |= bitMap - 1;
            bitMap &= ~((1L << x) - 1);
        }

        public long getBitMap() {
            return bitMap;
        }

        public long getBitMapLeaf(long bitPos) {
            // simple solution for readability (not that efficient as for each call a child is created again)
            return getChildNode(bitPos).getBitMap();
        }

        public BBTrieIntRange getChildNode(long bitPos) {
            int bitNum = Long.numberOfTrailingZeros(bitPos);
            if (x == y) {
                return new BBTrieIntRange(a, b, level - 1);
            } else if (bitNum == x) {
                return new BBTrieIntRange(a, ~0x0, level - 1);
            } else if (bitNum == y) {
                return new BBTrieIntRange(0, b, level - 1);
            } else {
                return new BBTrieIntRange(0, ~0x0, level - 1);
            }
        }

    }

    /** An interface that represents a methhod to call when visiting leaf nodes in a trie */
    public interface Visitor {
        public void visit(byte[] key, int keyLen);
    }

    /**
      * Recursively visits every node in the trie rooted at the given node and calls the given visitor's visit method on every leaf.
      *
      * @param node The root node.
      * @param visitor The visitor the call the visit method on.
      * @param key A byte array to track which key is being visited.
      * @param off Which part of the key to visit at this node.
      * @param len The length of the keys stored in the trie.
      */
    public static void visit(BBTrieNode node, Visitor visitor, byte[] key, int off, int len) {
        long bitMap = node.getBitMap();
        if (bitMap == 0) {
            return;
        }
        long bits = bitMap;
        while (bits != 0) {
            long bitPos = bits & -bits; bits ^= bitPos; // get rightmost bit and clear it
            int bitNum = Long.numberOfTrailingZeros(bitPos);
            key[off] = (byte) bitNum;
            if (off == len - 2) {
                long value = node.getBitMapLeaf(bitPos);
                long bits2 = value;
                while (bits2 != 0) {
                    long bitPos2 = bits2 & -bits2; bits2 ^= bitPos2;
                    int bitNum2 = Long.numberOfTrailingZeros(bitPos2);
                    key[off+1] = (byte) bitNum2;
                    visitor.visit(key, off + 2);
                }
            } else {
                BBTrieNode childNode = node.getChildNode(bitPos);
                visit(childNode, visitor, key, off + 1, len);
            }                
        }
    }

}
