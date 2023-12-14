/** Sample code for how to use CFG. */
public class CFGSample {

    public static void main(String[] args) {

        CFG cfg;

        try {
            String filename = args[0];
            cfg = CFG.fromMrRepairFile(filename);

            long startTime, endTime, duration = 0;
            char c;
            for (int i = 0; i < cfg.textLength(); i++) {
                startTime = System.nanoTime();
                c = cfg.get(i);
                endTime = System.nanoTime();

                System.out.print(c);

                duration += (endTime - startTime);  //divide by 1000000 to get milliseconds.

            }
            System.err.println("average query time: " + (duration / cfg.textLength()) + " ns");
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
        }
    }
}

