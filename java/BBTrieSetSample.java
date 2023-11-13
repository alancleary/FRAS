/** Sample code for how to use BBTrieSet and BBTrieSetOps. */
public class BBTrieSetSample {

    /**
      * Converts an int to a 6 byte key.
      *
      * @param b The array to store the key in.
      * @param value The int to convert into a key.
      * @return The number of bytes in the key (always 6).
      */
    public static int set6Int(byte[] b, int value) {
        int pos = 0;
        b[pos] = (byte) ((value >>> 30) & 0x3F);
        b[pos + 1] = (byte) ((value >>> 24) & 0x3F);
        b[pos + 2] = (byte) ((value >>> 18) & 0x3F);
        b[pos + 3] = (byte) ((value >>> 12) & 0x3F);
        b[pos + 4] = (byte) ((value >>> 6) & 0x3F);
        b[pos + 5] = (byte) (value & 0x3F);
        return 6;
    }

    /**
      * Converts a 6 byte key to its int value.
      *
      * @param b The key to convert.
      * @return The int value of the key.
      */
    public static int get6Int(byte[] b) {
        int pos = 0;
        return ((b[pos] & 0x3F) << 30) |
               ((b[pos + 1] & 0x3F) << 24) |
               ((b[pos + 2] & 0x3F) << 18) |
               ((b[pos + 3] & 0x3F) << 12) |
               ((b[pos + 4] & 0x3F) << 6) |
               (b[pos + 5] & 0x3F);
    }

    public static void main(String[] args) {
        // create a couple tries
        BBTrieSet trie1 = new BBTrieSet(100);
        BBTrieSet trie2 = new BBTrieSet(100);

        // create reusable key variables
        byte[] key = new byte[64];
        int len;
        final int KEY_LEN_INT = set6Int(key, 1); // 6

        // add keys to trie1
        System.out.println("add operation: trie1");
        int[] test = new int[] { 10, 20, 30, 40, 50, 30, 60, 61, 62, 63 };
        for (int i = 0; i < test.length; i++) {
            len = set6Int(key, test[i]);
            boolean change = trie1.set(key, len);
            System.out.println("set: " + test[i] + ", " + change);
        }
        System.out.println("trie1 size: " + trie1.size());

        // visit every node (key?) in trie1
        BBTrieSetOps.visit(
            new BBTrieSetOps.BBTrieNodeMem(trie1.root, trie1.mem),
            new BBTrieSetOps.Visitor() {
                @Override
                public void visit(byte[] key, int keyLen) {
                    System.out.println("Visitor: "+ get6Int(key) + ", " + keyLen);
                }
            },
            key, 0, KEY_LEN_INT);
        System.out.println("");

        // check if keys exist in trie1
        System.out.println("contains operation: trie1");
        test = new int[] { 10, 25, 30, 40, 45, 50, 55, 60 };
        for (int i = 0; i < test.length; i++) {
            len = set6Int(key, test[i]);
            boolean contained = trie1.get(key, len);
            System.out.println("contained: " + test[i] + ", " + contained);
        }
        System.out.println("");

        // clear (remove) values from trie1
        System.out.println("remove operation: trie1");
        test = new int[] { 10, 20, 30, 40, 45, 50, 55, 60, 61, 62, 63 };
        for (int i = 0; i < test.length; i++) {
            len = set6Int(key, test[i]);
            boolean change = trie1.clear(key, len);
            System.out.println("cleared: " + test[i] + ", " + change);
            BBTrieSetOps.visit(
                new BBTrieSetOps.BBTrieNodeMem(trie1.root, trie1.mem),
                new BBTrieSetOps.Visitor() {
                    @Override
                    public void visit(byte[] key, int keyLen) {
                        System.out.print(get6Int(key) + " ");
                    }
                },
                key, 0, KEY_LEN_INT);
            System.out.println();

        }
        System.out.println("trie1 size: " + trie1.size());
        System.out.println("");

        // add keys 0 through 50 to trie1
        System.out.println("add operation: trie1");
        for (int i = 0; i <= 50; i++) {
            len = set6Int(key, i);
            trie1.set(key, len);
            System.out.println("set: " + i);
        }
        System.out.println("trie1 size: " + trie1.size());
        System.out.println("");

        // add keys 25 through 75 to trie2
        System.out.println("add operation: trie2");
        for (int i = 25; i <= 75; i++) {
            len = set6Int(key, i);
            trie2.set(key, len);
            System.out.println("set: " + i);
        }
        System.out.println("trie2 size: " + trie2.size());
        System.out.println("");

        // AND example
        System.out.println("trie1 AND trie2");
        BBTrieSetOps.BBTrieNode andResult = new BBTrieSetOps.BBTrieAnd(
            new BBTrieSetOps.BBTrieNodeMem(trie1.root, trie1.mem),
            new BBTrieSetOps.BBTrieNodeMem(trie2.root, trie2.mem));

        // visit all the nodes in the AND results tree
        BBTrieSetOps.visit(
            andResult,
            new BBTrieSetOps.Visitor() {
                @Override
                public void visit(byte[] key, int keyLen) {
                    System.out.println("Visitor AND result: " + get6Int(key));
                }
            },
            key, 0, KEY_LEN_INT);
        System.out.println("");

        // OR example
        System.out.println("trie1 OR trie2");
        BBTrieSetOps.BBTrieNode orResult = new BBTrieSetOps.BBTrieOr(
            new BBTrieSetOps.BBTrieNodeMem(trie1.root, trie1.mem),
            new BBTrieSetOps.BBTrieNodeMem(trie2.root, trie2.mem));

        // visit all the nodes in the OR results tree
        BBTrieSetOps.visit(
            orResult,
            new BBTrieSetOps.Visitor() {
                @Override
                public void visit(byte[] key, int keyLen) {
                    System.out.println("Visitor OR result: " + get6Int(key));
                }
            },
            key, 0, KEY_LEN_INT);
        System.out.println("");

        // MINUS example
        System.out.println("trie1 MINUS trie2");
        BBTrieSetOps.BBTrieNode minusResult = new BBTrieSetOps.BBTrieMinus(
            new BBTrieSetOps.BBTrieNodeMem(trie1.root, trie1.mem),
            new BBTrieSetOps.BBTrieNodeMem(trie2.root, trie2.mem));

        // visit all the nodes in the MINUS results tree
        BBTrieSetOps.visit(
            minusResult,
            new BBTrieSetOps.Visitor() {
                @Override
                public void visit(byte[] key, int keyLen) {
                    System.out.println("Visitor OR result: " + get6Int(key));
                }
            },
            key, 0, KEY_LEN_INT);
        System.out.println("");

        // RANGE example
        System.out.println("trie1 AND range{10,...,50}");
        BBTrieSetOps.BBTrieNode rangeResult = new BBTrieSetOps.BBTrieAnd(
            new BBTrieSetOps.BBTrieNodeMem(trie1.root, trie1.mem),
            new BBTrieSetOps.BBTrieIntRange(10, 50));

        // visit all the nodes in the RANGE results tree
        BBTrieSetOps.visit(
            rangeResult,
            new BBTrieSetOps.Visitor() {
                @Override
                public void visit(byte[] key, int keyLen) {
                    System.out.println("Visitor RANGE result: " + get6Int(key));
                }
            },
            key, 0, KEY_LEN_INT);
        System.out.println("");

    }
}

