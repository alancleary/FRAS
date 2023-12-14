import java.io.BufferedReader;
import java.io.FileReader;


/** A representation of a context-free grammar built on BBTrieMap. */
public class CFG {

    private int textLength;
    private int numRules;
    private BBTrieMap map;

    final static int CHAR_SIZE = 256;

    public CFG() {
        map = new BBTrieMap(100);
    }

    /**
     * Converts an int to a 6 byte key.
     *
     * @param key The array to store the key in.
     * @param value The int to convert into a key.
     * @return The number of bytes in the key (always 6).
     */
    public static int set6Int(byte[] key, int value) {
        int pos = 0;
        key[pos] = (byte) ((value >>> 30) & 0x3F);
        key[pos + 1] = (byte) ((value >>> 24) & 0x3F);
        key[pos + 2] = (byte) ((value >>> 18) & 0x3F);
        key[pos + 3] = (byte) ((value >>> 12) & 0x3F);
        key[pos + 4] = (byte) ((value >>> 6) & 0x3F);
        key[pos + 5] = (byte) (value & 0x3F);
        return 6;
    }

    /**
     * Converts a 6 byte key to its int value.
     *
     * @param key The key to convert.
     * @return The int value of the key.
     */
    public static int get6Int(byte[] key) {
        int pos = 0;
        return ((key[pos] & 0x3F) << 30) |
               ((key[pos + 1] & 0x3F) << 24) |
               ((key[pos + 2] & 0x3F) << 18) |
               ((key[pos + 3] & 0x3F) << 12) |
               ((key[pos + 4] & 0x3F) << 6) |
               (key[pos + 5] & 0x3F);
    }

    final static int MR_REPAIR_CHAR_SIZE = 256;
    final static int MR_REPAIR_DUMMY_CODE = -1;  // UINT_MAX in MR-RePair C code

    /*
    public static void printMrRepairRules(int rules[][]) {
        int textLength = printMrRepairRules(rules, rules.length-1, 0);
        System.out.println();
        for (int i = 0; i < textLength; i++) {
            System.out.print(i + " ");
        }
        System.out.println();
    }

    public static int printMrRepairRules(int rules[][], int rule, int pos) {
        for (int i = 0; i < rules[rule].length; i++) {
            if (rules[rule][i] < MR_REPAIR_CHAR_SIZE) {
                System.out.print((char) rules[rule][i]);
                String s = Integer.toString(pos);
                for (int j = 0; j < s.length(); j++) {
                    System.out.print(" ");
                }
                pos += 1;
            } else {
                pos = printMrRepairRules(rules, rules[rule][i], pos);
            }
        }
        return pos;
    }
    */

    /**
     * Loads an MR-Repair grammar from a file.
     *
     * @param filename The file to load the grammar from.
     * @return The grammar that was loaded.
     * @throws Exception if the file cannot be read.
     */
    public static CFG fromMrRepairFile(String filename) throws Exception {
        CFG cfg = new CFG();

        BufferedReader reader = new BufferedReader(new FileReader(filename));

        // read grammar specs
        cfg.textLength = Integer.parseInt(reader.readLine());
        cfg.numRules = Integer.parseInt(reader.readLine());
        int startSize = Integer.parseInt(reader.readLine());

        // prepare to read grammar
        //int buff[] = new int[1024];
        int buffSize = cfg.numRules + MR_REPAIR_CHAR_SIZE;
        int rules[][] = new int[buffSize + 1][];  // +1 for start rule
        rules[buffSize] = new int[startSize];
        int i, j, c, ruleLength;

        // read rules in order they were added to grammar, i.e. line-by-line
        for (i = MR_REPAIR_CHAR_SIZE; i < buffSize; i++) {
            for (j = 0; ;j++) {
                c = Integer.parseInt(reader.readLine());
                if (c == MR_REPAIR_DUMMY_CODE) {
                    break;
                }
                //buff[j] = c;
                // use start rule as a buffer
                rules[buffSize][j] = c;
            }
            ruleLength = j;
            rules[i] = new int[ruleLength];
            for (j = 0; j < ruleLength; j++) {
                //rules[i][j] = buff[j];
                rules[i][j] = rules[buffSize][j];
            }
        }

        // read start rule
        for (i = 0; i < startSize; i++) {
            rules[buffSize][i] = Integer.parseInt(reader.readLine());
        }

        /*
        for (i = MR_REPAIR_CHAR_SIZE; i < rules.length; i++) {
            System.out.print(i + " -> ");
            for (j = 0; j < rules[i].length; j++) {
                System.out.print(rules[i][j] + " ");
                if (rules[i][j] < MR_REPAIR_CHAR_SIZE) {
                    rules[rules[i][j]] = new int[0];
                }
            }
            System.out.println();
        }

        for (i = 0; i < MR_REPAIR_CHAR_SIZE; i++) {
            if (rules[i] != null) {
                System.out.println(i + " -> " + ((char) i));
            }
        }

        printMrRepairRules(rules);
        */

        // convert start rule (and recursively all other rules) into a BBTrieMap
        fromMrRepairRules(cfg, rules);

        return cfg;
    }

    /**
     * Encodes MR-Repair grammar rules loaded from a file in a BBTrieMap.
     *
     * NOTE: The given rules are destroyed as they are processed.
     *
     * @param cfg The CFG to load the grammar rules into.
     */
    private static void fromMrRepairRules(CFG cfg, int rules[][]) {
        // initialize recursion variables
        int startRule = rules.length - 1;
        int ruleSizes[] = new int[rules.length];
        Integer references[] = new Integer[rules.length];

        // build CFG recursively from start rule
        fromMrRepairRules(cfg, rules, startRule, ruleSizes, references, 0);
    }

    private static void fromMrRepairRules(CFG cfg, int rules[][], int ruleIdx, int ruleSizes[], Integer references[], int seqPos) {
        // add the rule to the map
        int rule[] = rules[ruleIdx];
        int c;
        byte[] key = new byte[64];
        int len;
        for (int i = 0; i < rule.length; i++) {
            c = rule[i];
            len = set6Int(key, seqPos);
            // this is the first occurrence of the (non-)terminal character
            if (references[c] == null) {
                // the character is a terminal
                if (c < MR_REPAIR_CHAR_SIZE) {
                    ruleSizes[c] = 1;
                    references[c] = c;  // MR_REPAIR_CHAR_SIZE == CHAR_SIZE
                    cfg.map.set(key, len, references[c]);
                // the character is a non-terminal 
                } else {
                    references[c] = CHAR_SIZE + seqPos;
                    fromMrRepairRules(cfg, rules, c, ruleSizes, references, seqPos);
                }
            } else {
                cfg.map.set(key, len, references[c]);
            }
            seqPos += ruleSizes[c];
            ruleSizes[ruleIdx] += ruleSizes[c];
        }

        // delete the rule since it's no longer needed
        rules[ruleIdx] = null;
    }

    public int textLength() {
        return textLength;
    }

    public int numRules() {
        return numRules;
    }

    /**
      * Gets the character at position i in the original string.
      *
      * @param i The position in the original string.
      * @return The decoded character.
      * @throws Exception if i is out of bounds.
      */
    public char get(int i) throws Exception {
        if (i < 0 || i >= textLength) {
            throw new Exception("i is out of bounds");
        }

        byte[] key = new byte[64];

        int len = set6Int(key, i);
        int selected = (int) map.rankSelect(key, len);

        while (selected >= CHAR_SIZE) {
            i = (selected - CHAR_SIZE) + (i - get6Int(key));
            len = set6Int(key, i);
            selected = (int) map.rankSelect(key, len);
        }

        return (char) selected;
    }

    /**
      * Gets the substring at position i in the original string.
      *
      * @param i The position the substring starts at in the original string.
      * @param len The length of the substring to decode.
      * @return The decoded substring.
      * @throws Exception if i is out of bounds.
      */
    /*
    public String get(int i, int len) {
        if (i < 0 || i >= textLength) {
            throw new Exception("i is out of bounds");
        }
        if (i + len > textLength) {
            len = textLength - i;
        }

        byte[] key = new byte[64];
        int len;

        return "a";
    }
    */

}

