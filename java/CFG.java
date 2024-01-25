import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.HashMap;


/** A representation of a context-free grammar built on BBTrieMap. */
public class CFG {

    private int textLength;
    private int numRules;
    private int startSize;
    private int rulesSize;
    private int depth;
    private BBTrieMap map;

    final static int KEY_LENGTH = 6;
    final static int CHAR_SIZE = 256;

    public CFG() {
        map = new BBTrieMap(100);
    }

    private class PrintMapVisitor implements BBTrieMap.Visitor {

        public void visit(byte[] key, int len, long value) {
            int currentKey = get6Int(key);

            if (value < CHAR_SIZE) {
                System.out.print("(" + currentKey + ": " + ((char) value) + ") ");
            } else {
                System.out.print("(" + currentKey + ": " + (value - CHAR_SIZE) + ") ");
            }
        }
    }

    public void printCfg() {
        PrintMapVisitor v = new PrintMapVisitor();
        map.visit(v, KEY_LENGTH);
        System.out.println();
    }

    /**
     * Converts an int to a 6 byte key.
     *
     * @param key The array to store the key in.
     * @param value The int to convert into a key.
     * @return The number of bytes in the key (always 6).
     */
    public static int set6Int(byte[] key, int value) {
        return set6Int(key, value, 0);
    }

    public static int set6Int(byte[] key, int value, int pos) {
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
        return get6Int(key, 0);
    }

    public static int get6Int(byte[] key, int pos) {
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
        cfg.startSize = Integer.parseInt(reader.readLine());
        cfg.rulesSize = 0;

        // prepare to read grammar
        //int buff[] = new int[1024];
        int buffSize = cfg.numRules + MR_REPAIR_CHAR_SIZE;
        int rules[][] = new int[buffSize + 1][];  // +1 for start rule
        rules[buffSize] = new int[cfg.startSize];
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
            cfg.rulesSize += ruleLength;
            rules[i] = new int[ruleLength];
            for (j = 0; j < ruleLength; j++) {
                //rules[i][j] = buff[j];
                rules[i][j] = rules[buffSize][j];
            }
        }

        // read start rule
        for (i = 0; i < cfg.startSize; i++) {
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
        int ruleDepths[] = new int[rules.length];
        Integer references[] = new Integer[rules.length];

        // build CFG recursively from start rule
        fromMrRepairRules(cfg, rules, startRule, ruleSizes, ruleDepths, references, 0);

        // save the depth
        cfg.depth = ruleDepths[startRule] - 1;
    }

    private static void fromMrRepairRules(CFG cfg, int rules[][], int ruleIdx, int ruleSizes[], int ruleDepths[] , Integer references[], int seqPos) {
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
                    ruleDepths[c] = 1;
                    references[c] = c;  // MR_REPAIR_CHAR_SIZE == CHAR_SIZE
                    cfg.map.set(key, len, references[c]);
                // the character is a non-terminal 
                } else {
                    references[c] = CHAR_SIZE + seqPos;
                    fromMrRepairRules(cfg, rules, c, ruleSizes, ruleDepths, references, seqPos);
                }
            } else {
                cfg.map.set(key, len, references[c]);
            }
            seqPos += ruleSizes[c];
            ruleSizes[ruleIdx] += ruleSizes[c];
            ruleDepths[ruleIdx] = Math.max(ruleDepths[ruleIdx], ruleDepths[c] + 1);
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

    public int startSize() {
        return startSize;
    }

    public int rulesSize() {
        return rulesSize;
    }

    public int totalSize() {
        return startSize + rulesSize;
    }

    public int depth() {
        return depth;
    }

    public long numMapEntries() {
        return map.size();
    }

    public long mapSize() {
        return map.nodeCount(KEY_LENGTH);
    }

    private class TailVisitor implements BBTrieMap.TailVisitor {
        int totalTailLength = 0;
        public void visit(byte[] key, int len, long value, int tailLength) {
            this.totalTailLength += tailLength;
        }
    }

    public int mapTailSize() {
        TailVisitor visitor = new TailVisitor();
        map.tailVisit(visitor, KEY_LENGTH);
        return visitor.totalTailLength * 2;
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

        byte[] key = new byte[KEY_LENGTH];

        int len = set6Int(key, i);
        int selected = (int) map.predecessor(key, len);

        while (selected >= CHAR_SIZE) {
            i = (selected - CHAR_SIZE) + (i - get6Int(key));
            len = set6Int(key, i);
            selected = (int) map.predecessor(key, len);
        }

        return (char) selected;
    }

    /**
      * Gets a substring in the original string.
      *
      * @param out The output stream to write the substring to.
      * @param begin The start position of the substring in the original string.
      * @param end The end position of the substring in the original string.
      * @throws Exception if begin or end is out of bounds.
      */
    public void get(OutputStreamWriter out, int begin, int end) throws Exception {
        if (begin < 0 || end >= textLength || begin > end) {
            throw new Exception("begin/end out of bounds");
        }

        byte[] key = new byte[KEY_LENGTH * depth()];
        set6Int(key, begin);
        int value = (int) map.predecessor(key, KEY_LENGTH);
        int predecessor = get6Int(key);
        int ignore = begin - predecessor;

        OutputStreamFilter filteredOut = new OutputStreamFilter(out, ignore);
        GetVisitor visitor = new GetVisitor(filteredOut);

        get(visitor, key, predecessor, end);
    }

    /** A wrapper around an output stream that ignores a number of characters. */
    private class OutputStreamFilter {

        private OutputStreamWriter out;
        private int ignore;

        public OutputStreamFilter(OutputStreamWriter out, int ignore) {
            this.out = out;
            this.ignore = ignore;
        }

        public void write(char c) throws IOException {
            if (ignore > 0) {
                ignore--;
            } else {
                out.write(c);
            }
        }
    }

    /** A simple cache for strings generated by grammar rules. */
    private class RuleCache {

        public static class Entry {
            char[] chars;
            int offset;
        }

        // maps rules to the strings they generate
        HashMap<Integer, char[]> strings = new HashMap<Integer, char[]>();
        // maps nested rules into the string of a rule they're nested in
        // value always has length 3: [0] = rule, [1] = start
        HashMap<Integer, int[]> pointers = new HashMap<Integer, int[]>();

        public void getRule(int rule, Entry entry) {
            // attempt to get the string
            entry.chars = strings.get(rule);
            if (entry.chars != null) {
                entry.offset = 0;
                return;
            }

            // attempt to get a pointer to another string
            int[] pointer = pointers.get(rule);
            if (pointer == null) {
                return;
            }
            entry.chars = strings.get(pointer[0]);
            if (entry.chars == null) {
                pointers.remove(rule);
                return;
            }

            entry.offset = pointer[1];
        }

        /**
         * Creates a cache entry for a string generated by a rule. The string is populated outside
         * of the cache.
         *
         * @param rule The rule that generates the string.
         * @param length The length of the string.
         * @return The string to be populated.
         */
        public char[] putString(int rule, int length) {
            char[] chars = new char[length];
            strings.put(rule, chars);
            return chars;
        }

        /**
         * Removes the cache entry for a string generated by a rule..
         *
         * @param rule The rule that generates the string.
         * @return The string removed from the cache.
         */
        public char[] popString(int rule) {
            return strings.remove(rule);
        }

        /**
         * Creates an entry pointing to a string already in the cache.
         *
         * @param rule The rule that generates the string.
         * @param length The length of the string.
         * @return The string.
         */
        public void putPointer(int rule, int reference, int start) {
            pointers.put(rule, new int[]{reference, start});
        }

    }

    private class GetVisitor implements BBTrieMap.Visitor {

        OutputStreamFilter out;

        RuleCache cache = new RuleCache();
        RuleCache.Entry cacheEntry = new RuleCache.Entry();
        int currentlyCaching;
        char[] currentCache;
        int currentCachePosition;

        int previousKey = -1;
        int previousValue;
        int keyOffset = 0;

        //public GetVisitor(OutputStreamWriter out) {
        public GetVisitor(OutputStreamFilter out) {
            this.out = out;
        }

        private void newCacheEntry(int rule, int length) {
            if (this.currentCache == null) {
                this.currentlyCaching = rule;
                this.currentCache = this.cache.putString(rule, length);
                this.currentCachePosition = 0;
            } else {
                this.cache.putPointer(rule, this.currentlyCaching, this.currentCachePosition);
            }
        }

        private void writeChar(char c) {
            try {
                this.out.write(c);
                if (this.currentCache != null) {
                    this.currentCache[this.currentCachePosition++] = (char) this.previousValue;
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        public void processPrevious(byte[] key, int currentKey) {
            // no rule to process
            if (this.previousKey < 0) {
                return;
            }

            // the rule is a terminal character
            if (this.previousValue < CHAR_SIZE) {
                this.writeChar((char) this.previousValue);
                return;
            }

            // the rule is cached
            this.cache.getRule(this.previousValue, this.cacheEntry);
            int length = currentKey - this.previousKey;
            if (this.cacheEntry.chars != null) {
                int i = 0;
                for (; i < Math.min(length, this.cacheEntry.chars.length - this.cacheEntry.offset); i++) {
                    this.writeChar(this.cacheEntry.chars[this.cacheEntry.offset + i]);
                }
                if (i < length) {
                    this.previousKey += i;
                    this.previousValue += i;
                    this.processPrevious(key, currentKey);
                }
            } else {
                // create a new cache entry and recursively generate the string
                int begin = this.previousValue - CHAR_SIZE;
                int end = begin + length - 1;

                this.newCacheEntry(this.previousValue, length);

                this.previousKey = -1;
                this.keyOffset += KEY_LENGTH;
                get(this, key, begin, end);
                this.keyOffset -= KEY_LENGTH;
            }

            // only non-terminals in the start rule are cached directly
            if (this.keyOffset == 0) {
                this.currentCache = null;
            }
        }

        public void visit(byte[] key, int len, long value) {
            int currentKey = get6Int(key, this.keyOffset);

            this.processPrevious(key, currentKey);

            this.previousKey = currentKey;
            this.previousValue = (int) value;
        }
    }

    /** Gets the substring after the head. */
    private void get(GetVisitor visitor, byte[] key, int begin, int end) {

        map.visitRange(visitor, key, visitor.keyOffset, begin, end, KEY_LENGTH);

        // process the interval between the last value visited and end
        visitor.processPrevious(key, end + 1);  // +1 to include the end
        visitor.previousKey = -1;
    }

}

