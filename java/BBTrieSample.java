/** Sample code for how to use BBTrieSet, BBTrieSetOps, and BBTrieMap. */
public class BBTrieSample {

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
        // create reusable key variables
        int[] test;
        byte[] key = new byte[64];
        int len;
        final int KEY_LEN_INT = set6Int(key, 1); // 6

        // create a couple sets
        BBTrieSet set1 = new BBTrieSet(100);
        BBTrieSet set2 = new BBTrieSet(100);

        // add keys to set1
        System.out.println("add operation: set1");
        test = new int[] { 10, 20, 30, 40, 50, 30, 60, 61, 62, 63 };
        for (int i = 0; i < test.length; i++) {
            len = set6Int(key, test[i]);
            boolean change = set1.set(key, len);
            System.out.println("set: " + test[i] + ", " + change);
        }
        System.out.println("set1 size: " + set1.size());
        System.out.println();

        // visit all nodes in set1
        BBTrieSetOps.visit(
            new BBTrieSetOps.BBTrieNodeMem(set1.root, set1.mem),
            new BBTrieSetOps.Visitor() {
                @Override
                public void visit(byte[] key, int keyLen) {
                    System.out.println("Visitor: "+ get6Int(key) + ", " + keyLen);
                }
            },
            key, 0, KEY_LEN_INT);
        System.out.println();

        // check if keys exist in set1
        System.out.println("contains operation: set1");
        //test = new int[] { 10, 25, 30, 40, 45, 50, 55, 60 };
        for (int i = 0; i < test.length; i++) {
            len = set6Int(key, test[i]);
            boolean contained = set1.get(key, len);
            System.out.println("contained: " + test[i] + ", " + contained);
        }
        System.out.println();

        // predecessor queries on set1
        System.out.println("predecessor operation: set1");
        test = new int[] { 0, 10, 25, 30, 40, 45, 50, 55, 60, 70 };
        for (int i = 0; i < test.length; i++) {
            len = set6Int(key, test[i]);
            boolean selected = set1.predecessor(key, len);
            System.out.println("selected: " + test[i] +" -> " + get6Int(key) + ", " + selected);
        }
        System.out.println();

        // successor queries on set1
        System.out.println("successor operation: set1");
        for (int i = 0; i < test.length; i++) {
            len = set6Int(key, test[i]);
            boolean selected = set1.successor(key, len);
            System.out.println("selected: " + test[i] +" -> " + get6Int(key) + ", " + selected);
        }
        System.out.println();

        // clear (remove) values from set1
        System.out.println("remove operation: set1");
        test = new int[] { 10, 20, 30, 40, 45, 50, 55, 60, 61, 62, 63 };
        for (int i = 0; i < test.length; i++) {
            len = set6Int(key, test[i]);
            boolean change = set1.clear(key, len);
            System.out.println("cleared: " + test[i] + ", " + change);
            BBTrieSetOps.visit(
                new BBTrieSetOps.BBTrieNodeMem(set1.root, set1.mem),
                new BBTrieSetOps.Visitor() {
                    @Override
                    public void visit(byte[] key, int keyLen) {
                        System.out.print(get6Int(key) + " ");
                    }
                },
                key, 0, KEY_LEN_INT);
            System.out.println();

        }
        System.out.println("set1 size: " + set1.size());
        System.out.println();

        // add keys 0 through 50 to set1
        System.out.println("add operation: set1");
        for (int i = 0; i <= 50; i++) {
            len = set6Int(key, i);
            set1.set(key, len);
            System.out.println("set: " + i);
        }
        System.out.println("set1 size: " + set1.size());
        System.out.println();

        // add keys 25 through 75 to set2
        System.out.println("add operation: set2");
        for (int i = 25; i <= 75; i++) {
            len = set6Int(key, i);
            set2.set(key, len);
            System.out.println("set: " + i);
        }
        System.out.println("set2 size: " + set2.size());
        System.out.println();

        // AND example
        System.out.println("set1 AND set2");
        BBTrieSetOps.BBTrieNode andResult = new BBTrieSetOps.BBTrieAnd(
            new BBTrieSetOps.BBTrieNodeMem(set1.root, set1.mem),
            new BBTrieSetOps.BBTrieNodeMem(set2.root, set2.mem));

        // visit all the nodes in the AND results set
        BBTrieSetOps.visit(
            andResult,
            new BBTrieSetOps.Visitor() {
                @Override
                public void visit(byte[] key, int keyLen) {
                    System.out.println("Visitor AND result: " + get6Int(key));
                }
            },
            key, 0, KEY_LEN_INT);
        System.out.println();

        // OR example
        System.out.println("set1 OR set2");
        BBTrieSetOps.BBTrieNode orResult = new BBTrieSetOps.BBTrieOr(
            new BBTrieSetOps.BBTrieNodeMem(set1.root, set1.mem),
            new BBTrieSetOps.BBTrieNodeMem(set2.root, set2.mem));

        // visit all the nodes in the OR results set
        BBTrieSetOps.visit(
            orResult,
            new BBTrieSetOps.Visitor() {
                @Override
                public void visit(byte[] key, int keyLen) {
                    System.out.println("Visitor OR result: " + get6Int(key));
                }
            },
            key, 0, KEY_LEN_INT);
        System.out.println();

        // MINUS example
        System.out.println("set1 MINUS set2");
        BBTrieSetOps.BBTrieNode minusResult = new BBTrieSetOps.BBTrieMinus(
            new BBTrieSetOps.BBTrieNodeMem(set1.root, set1.mem),
            new BBTrieSetOps.BBTrieNodeMem(set2.root, set2.mem));

        // visit all the nodes in the MINUS results set
        BBTrieSetOps.visit(
            minusResult,
            new BBTrieSetOps.Visitor() {
                @Override
                public void visit(byte[] key, int keyLen) {
                    System.out.println("Visitor OR result: " + get6Int(key));
                }
            },
            key, 0, KEY_LEN_INT);
        System.out.println();

        // RANGE example
        System.out.println("set1 AND range{10,...,50}");
        BBTrieSetOps.BBTrieNode rangeResult = new BBTrieSetOps.BBTrieAnd(
            new BBTrieSetOps.BBTrieNodeMem(set1.root, set1.mem),
            new BBTrieSetOps.BBTrieIntRange(10, 50));

        // visit all the nodes in the RANGE results set
        BBTrieSetOps.visit(
            rangeResult,
            new BBTrieSetOps.Visitor() {
                @Override
                public void visit(byte[] key, int keyLen) {
                    System.out.println("Visitor RANGE result: " + get6Int(key));
                }
            },
            key, 0, KEY_LEN_INT);
        System.out.println();

        // create a map
        BBTrieMap map1 = new BBTrieMap(100);

        // add keys and values to map1
        System.out.println("add operation: map1");
        test = new int[] { 10, 20, 30, 40, 50, 30, 60, 61, 62, 63 };
        System.out.println();
        for (int i = 0; i < test.length; i++) {
            len = set6Int(key, test[i]);
            boolean change = map1.set(key, len, test[i]);
            System.out.println("set: " + test[i] + ", " + change);
        }
        System.out.println("map1 size: " + map1.size());
        System.out.println();

        // check if keys exist in map1
        System.out.println("contains operation: map1");
        test = new int[] { 10, 20, 25, 30, 40, 45, 50, 55, 60, 61, 62, 63 };
        for (int i = 0; i < test.length; i++) {
            len = set6Int(key, test[i]);
            try {
                long contained = map1.get(key, len);
                System.out.println("contained: " + test[i] + ", " + contained);
            } catch (Exception e) {
                System.out.println(test[i] + " not contained");
            }
        }
        System.out.println();

        // predecessor queries on map1
        System.out.println("predecessor operation: map1");
        test = new int[] { 0, 10, 25, 30, 40, 45, 50, 55, 60, 70 };
        for (int i = 0; i < test.length; i++) {
            len = set6Int(key, test[i]);
            try {
                long selected = map1.predecessor(key, len);
                System.out.println("selected: " + test[i] +" -> " + get6Int(key) + ", " + selected);
            } catch (Exception e) {
                System.out.println(test[i] + " not selected");
            }
        }
        System.out.println();

        // predecessor queries on map1
        System.out.println("sucessor operation: map1");
        for (int i = 0; i < test.length; i++) {
            len = set6Int(key, test[i]);
            try {
                long selected = map1.successor(key, len);
                System.out.println("selected: " + test[i] +" -> " + get6Int(key) + ", " + selected);
            } catch (Exception e) {
                System.out.println(test[i] + " not selected");
            }
        }
        System.out.println();

        // clear (remove) keys from map1
        System.out.println("remove operation: map1");
        //test = new int[] { 10, 20, 30, 40, 45, 50, 55, 60, 61, 62, 63 };
        for (int i = 0; i < test.length; i++) {
            len = set6Int(key, test[i]);
            boolean change = map1.clear(key, len);
            System.out.println("clear: " + test[i] + ", " + change);
            for (int j = i; j < test.length; j++) {
                len = set6Int(key, test[j]);
                try {
                    long contained = map1.get(key, len);
                    System.out.println("\tcontained: " + test[j] + ", " + contained);
                } catch (Exception e) {
                    System.out.println("\t" + test[j] + " not contained");
                }
            }
        }
        System.out.println("map1 size: " + map1.size());
        System.out.println();
    }
}

