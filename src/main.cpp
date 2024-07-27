#include <iostream>
#include <chrono>
#include <vector>

#include "cfg/cfg.hpp"
//#include "cfg/random_access_amt.hpp"
//#include "cfg/random_access_bv.hpp"
//#include "cfg/random_access_v2_bv.hpp"
#include "cfg/random_access_v2_sd.hpp"
#include "xoroshiro/xoroshiro128plus.hpp"

using namespace std;
using namespace cfg;

void usage(int argc, char* argv[]) {
    cerr << "usage: " << argv[0] << " <type> <filename> <querysize> [numqueries=10000] [seed=random_device]" << endl;
    cerr << endl;
    cerr << "args: " << endl;
    cerr << "\ttype={mrrepair|navarro|bigrepair}: the type of grammar to load" << endl;
    cerr << "\t\tmrrepair: for grammars created with the MR-RePair algorithm" << endl;
    cerr << "\t\tnavarro: for grammars created with Navarro's implementation of RePair" << endl;
    cerr << "\t\tbigrepair: for grammars created with Manzini's implementation of Big-Repair" << endl;
    cerr << "\tfilename: the name of the grammar file(s) without the extension(s)" << endl;
    cerr << "\tquerysize: the size of the substring to query for when benchmarking" << endl;
    cerr << "\tnumqueries: the number of queries to run when benchmarking" << endl;
    cerr << "\tseed: the seed to use with the pseudo-random number generator" << endl;
}

CFG* loadGrammar(string type, string filename) {
    if (type == "mrrepair") {
        return CFG::fromMrRepairFile(filename + ".out");
    } else if (type == "navarro") {
        return CFG::fromNavarroFiles(filename + ".C", filename + ".R");
    } else if (type == "bigrepair") {
        return CFG::fromBigRepairFiles(filename + ".C", filename + ".R");
    }
    cerr << "invalid grammar type: \"" << type << "\"" << endl;
    cerr << endl;
    return NULL;
}

int main(int argc, char* argv[])
{

    // check the command-line arguments
    if (argc < 4) {
      usage(argc, argv);
      return 1;
    }

    // get benchmark parameters
    uint32_t querySize = std::stoi(argv[3]);
    uint32_t numQueries = 10000;
    if (argc >= 5) {
      numQueries = std::stoi(argv[4]);
    }

    // setup the pseudo-random number generator
    xoroshiro::xoroshiro128plus_engine eng;
    if (argc >= 6) {
      eng.seed([&argv]() { return std::stoi(argv[5]); });
    } else {
      std::random_device dev{};
      eng.seed([&dev]() { return dev(); });
    }
    std::uniform_real_distribution<> dist(0.0, 1.0);

    // load the grammar
    string type = argv[1];
    string filename = argv[2];
    CFG* cfg = loadGrammar(type, filename);

    // print grammar stats
    cerr << "text length: " << cfg->getTextLength() << endl;
    cerr << "num rules: " << cfg->getNumRules() << endl;
    cerr << "start size: " << cfg->getStartSize() << endl;
    cerr << "rules size: " << cfg->getRulesSize() << endl;
    cerr << "total size: " << cfg->getTotalSize() << endl;
    cerr << "depth: " << cfg->getDepth() << endl;
    uint64_t cfgMemSize = cfg->memSize();
    cerr << "mem size: " << cfgMemSize << endl;

    // instantiate indexes
    //RandomAccessAMT amt(cfg);
    //RandomAccessBV<sdsl::bit_vector, sdsl::rank_support_v5<>, sdsl::select_support_mcl<>> bv(cfg);
    //RandomAccessV2BV<sdsl::bit_vector, sdsl::rank_support_v5<>, sdsl::select_support_mcl<>> bv2(cfg);
    RandomAccessV2SD sd(cfg);
    uint64_t sdMemSize = sd.memSize();
    cerr << "sdv2 mem size: " << sdMemSize << endl;

    cerr << "total mem size: " << cfgMemSize + sdMemSize << endl;
    
    // generate the original text
    //cfg->get(cout, 0, cfg->getTextLength() - 1);

    // benchmarks
    chrono::steady_clock::time_point startTime, endTime;
    //uint64_t durationAMT = 0;
    //uint64_t durationBV = 0;
    //uint64_t durationBV2 = 0;
    //uint64_t durationSD = 0;
    uint32_t numLoops = 11;

    uint64_t begin, end;
    char* out = new char[querySize];
    std::vector<double> times(numLoops);

    //cout.setstate(std::ios::failbit);
    for (int i = 0; i < numLoops; i++) {
      double durationSD = 0;
      for (int j = 0; j < numQueries; j++) {
          //begin = distr(gen);
=======
          uint64_t tester = cfg->getTextLength();
          uint64_t tester2 = querySize;
          uint64_t tester3 = dist(eng);
          begin = (cfg->getTextLength() - querySize) * dist(eng);
          //std::cerr << begin << std::endl;
          end = begin + querySize - 1;

          // AMT
          //startTime = chrono::steady_clock::now();
          //amt.get(cout, begin, end);
          //endTime = chrono::steady_clock::now();
          //durationAMT += chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();

          // BV
          //startTime = chrono::steady_clock::now();
          //bv.get(cout, begin, end);
          //endTime = chrono::steady_clock::now();
          //durationBV += chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();

          //begin = distr(gen);
          //end = begin + querySize - 1;

          // BV2
          //startTime = chrono::steady_clock::now();
          //bv2.get(cout, begin, end);
          //endTime = chrono::steady_clock::now();
          //durationBV2 += chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();

          // SD
          startTime = chrono::steady_clock::now();
          //sd.get(cout, begin, end);
          sd.get(out, begin, end);
          endTime = chrono::steady_clock::now();
          durationSD += chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();

          //begin = distr(gen);
          //end = begin + querySize - 1;
      }

      times[i] = durationSD / numQueries;
    }
    std::sort(times.begin(), times.end());

    //cerr << "average AMT query time: " << durationAMT / numQueries << "[µs]" << endl;

    //cerr << "average BV query time: " << durationBV / numQueries << "[µs]" << endl;

    //cerr << "average BV2 query time: " << durationBV2 / numQueries << "[µs]" << endl;

    cerr << "average SD query time: " << times[numLoops / 2] << "[µs]" << endl;

    delete[] out;
    delete cfg;

    return 1;
}
