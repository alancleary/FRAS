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
        b[pos    ] = (byte) ((value >>> 30) & 0x3F);
        b[pos + 1] = (byte) ((value >>> 24) & 0x3F);
        b[pos + 2] = (byte) ((value >>> 18) & 0x3F);
        b[pos + 3] = (byte) ((value >>> 12) & 0x3F);
        b[pos + 4] = (byte) ((value >>> 6) & 0x3F);
        b[pos + 5] = (byte) (value & 0x3F);
        return 6;
    }

    public static void main(String[] args) {
        // create a trie
        BBTrieSet trie1 = new BBTrieSet(100);

        // create reusable key variables
        byte[] key = new byte[64];
        int len;

        // add keys to the trie
        int[] test = new int[] { 10, 20, 30, 40, 50, 30, 60, 61, 62, 63 };
        for (int i = 0; i < test.length; i++) {
            len = set6Int(key, test[i]);
            boolean change = trie1.set(key, len);
            System.out.println("set: " + test[i] + ", " + change);
        }
        System.out.println("trie1 size: " + trie1.size());

        // check if keys exist in the tree
        test = new int[] { 10, 25, 30, 40, 45, 50, 55, 60 };
        for (int i = 0; i < test.length; i++) {
            len = set6Int(key, test[i]);
            boolean contained = trie1.get(key, len);
            System.out.println("contained: " + test[i] + ", " + contained);
        }

    }
}

