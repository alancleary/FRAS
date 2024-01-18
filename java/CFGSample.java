import java.io.OutputStreamWriter;
import java.util.Random;


/** Sample code for how to use CFG. */
public class CFGSample {

    public static void main(String[] args) {

        CFG cfg;

        try {
            String filename = args[0];
            cfg = CFG.fromMrRepairFile(filename);
            System.err.println("text length: " + cfg.textLength());
            System.err.println("num rules: " + cfg.numRules());
            System.err.println("depth: " + cfg.depth());

            //cfg.printCfg();

            OutputStreamWriter out = new OutputStreamWriter(System.out);

            Random r = new Random();
            int begin, end, numQueries = 10000, querySize = 1000;
            long startTime, endTime, duration = 0;

            /*
            for (int i = 0; i < cfg.textLength(); i++) {
                begin = r.nextInt(cfg.textLength() - querySize + 1);
                end = begin + querySize;

                startTime = System.nanoTime();
                for (int j = begin; j < end; j++) {
                    out.write(cfg.get(i));
                }
                endTime = System.nanoTime();
                duration += (endTime - startTime);  //divide by 1000000 to get milliseconds.

                out.flush();
            }
            System.err.println("average query time: " + (duration / numQueries) + " ns");
            */

            /*
            startTime = System.nanoTime();
            cfg.get(out, 0, cfg.textLength() - 1);
            endTime = System.nanoTime();
            System.err.println("query time: " + ((endTime - startTime) / 1000000) + " ms");
            out.flush();
            */

            for (int i = 0; i < numQueries; i++) {
                begin = r.nextInt(cfg.textLength() - querySize + 1);
                end = begin + querySize - 1;

                startTime = System.nanoTime();
                cfg.get(out, begin, end);
                endTime = System.nanoTime();
                duration += (endTime - startTime);

                out.flush();
                System.out.println();

            }
            // microseconds
            String average = ((duration / 1000) / numQueries) + " μs";
            // nanoseconds
            //String average = (duration / numQueries) + " ns";
            // picoseconds
            //String average = ((duration * 1000) / numQueries) + " ps";
            System.err.println("average query time: " + average);

        } catch (Exception e) {
            System.err.println(e);
            e.printStackTrace();
        }
    }
}

