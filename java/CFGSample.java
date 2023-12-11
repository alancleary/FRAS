/** Sample code for how to use CFG. */
public class CFGSample {

    public static void main(String[] args) {

        CFG cfg;

        try {
            String filename = args[0];
            cfg = CFG.fromMrRepairFile(filename);

            for (int i = 0; i < cfg.textLength; i++) {
                System.out.print(cfg.get(i));
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
        }
    }
}

