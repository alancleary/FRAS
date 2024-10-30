#include <iostream>
#include <chrono>
#include <vector>

#include "fras/array/jagged_array_int.hpp"
#include "fras/array/jagged_array_bp_index.hpp"
#include "fras/array/jagged_array_bp_mono.hpp"
#include "fras/array/jagged_array_bp_opt.hpp"
#include "fras/cfg/cfg.hpp"
//#include "fras/cfg/random_access_bv.hpp"
#include "fras/cfg/random_access_sd.hpp"
#include "xoroshiro/xoroshiro128plus.hpp"

using namespace std;
using namespace fras;

void usage(int argc, char* argv[]) {
    cerr << "usage: " << argv[0] << " <type> <filename> <encoding> <querysize> [numqueries=10000] [seed=random_device]" << endl;
    cerr << endl;
    cerr << "args: " << endl;
    cerr << "\ttype={mrrepair|navarro|bigrepair}: the type of grammar to load" << endl;
    cerr << "\t\tmrrepair: for grammars created with the MR-RePair algorithm" << endl;
    cerr << "\t\tnavarro: for grammars created with Navarro's implementation of RePair" << endl;
    cerr << "\t\tbigrepair: for grammars created with Manzini's implementation of Big-Repair" << endl;
    cerr << "\tfilename: the name of the grammar file(s) without the extension(s)" << endl;
    cerr << "\tecoding={array|bpleft|bpright|bpmono}: how the grammar should be encoded in memory" << endl;
    cerr << "\t\tarray: an array of arrays (fastest)" << endl;
    cerr << "\t\tbpleft: bit packing where the pack width of a rule is detrmined by its non-terminal character" << endl;
    cerr << "\t\tbpright: bit packing where the pack width of a rule is the smallest that is compatible with every character in the rule" << endl;
    cerr << "\t\tbpmono: same as bpright but a rule's pack width cannot be less than the previous rule's width" << endl;
    cerr << "\tquerysize: the size of the substring to query for when benchmarking" << endl;
    cerr << "\tnumqueries: the number of queries to run when benchmarking" << endl;
    cerr << "\tseed: the seed to use with the pseudo-random number generator" << endl;
}

template <class JaggedArray_T>
CFG<JaggedArray_T>* loadGrammar(string type, string filename) {
    if (type == "mrrepair") {
        return CFG<JaggedArray_T>::fromMrRepairFile(filename + ".out");
    } else if (type == "navarro") {
        return CFG<JaggedArray_T>::fromNavarroFiles(filename + ".C", filename + ".R");
    } else if (type == "bigrepair") {
        return CFG<JaggedArray_T>::fromBigRepairFiles(filename + ".C", filename + ".R");
    }
    cerr << "invalid grammar type: \"" << type << "\"" << endl;
    cerr << endl;
    return NULL;
}

template <class JaggedArray_T>
void benchmark(JaggedArray_T* cfg, uint32_t querySize, uint32_t numQueries, xoroshiro::xoroshiro128plus_engine& eng) {
    // print grammar stats
    cerr << "\ttext length: " << cfg->getTextLength() << endl;
    cerr << "\tnum rules: " << cfg->getNumRules() << endl;
    cerr << "\tstart size: " << cfg->getStartSize() << endl;
    cerr << "\trules size: " << cfg->getRulesSize() << endl;
    cerr << "\ttotal size: " << cfg->getTotalSize() << endl;
    cerr << "\tdepth: " << cfg->getDepth() << endl;
    uint64_t cfgMemSize = cfg->memSize();
    cerr << "\tmem size: " << cfgMemSize << endl;

    // instantiate indexes
    RandomAccessSD sd(cfg);
    uint64_t sdMemSize = sd.memSize();
    cerr << "\tsd mem size: " << sdMemSize << endl;

    cerr << "\ttotal mem size: " << cfgMemSize + sdMemSize << endl;
    
    // generate the original text
    //cfg->get(cout, 0, cfg->getTextLength() - 1);

    // benchmarks
    std::cerr << "running benchmarks..." << std::endl;
    chrono::steady_clock::time_point startTime, endTime;
    uint32_t numLoops = 11;

    std::uniform_real_distribution<> dist(0.0, 1.0);
    uint64_t begin, end;
    char* out = new char[querySize];
    std::vector<double> times(numLoops);

    //cout.setstate(std::ios::failbit);
    for (int i = 0; i < numLoops; i++) {
      double durationSD = 0;
      for (int j = 0; j < numQueries; j++) {
          begin = (cfg->getTextLength() - querySize) * dist(eng);
          end = begin + querySize - 1;
          startTime = chrono::steady_clock::now();
          //sd.get(cout, begin, end);
          sd.get(out, begin, end);
          endTime = chrono::steady_clock::now();
          durationSD += chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();
      }

      times[i] = durationSD / numQueries;
    }
    std::sort(times.begin(), times.end());

    cerr << "average SD query time: " << times[numLoops / 2] << "[µs]" << endl;

    delete[] out;
}

int main(int argc, char* argv[])
{

    // check the command-line arguments
    if (argc < 5) {
      usage(argc, argv);
      return 1;
    }

    // get benchmark parameters
    uint32_t querySize = std::stoi(argv[4]);
    uint32_t numQueries = 10000;
    if (argc >= 6) {
      numQueries = std::stoi(argv[5]);
    }

    // setup the pseudo-random number generator
    xoroshiro::xoroshiro128plus_engine eng;
    if (argc >= 7) {
      eng.seed([&argv]() { return std::stoi(argv[6]); });
    } else {
      std::random_device dev{};
      eng.seed([&dev]() { return dev(); });
    }

    // load the grammar
    std::cerr << "loading grammar..." << std::endl;
    string type = argv[1];
    string filename = argv[2];
    string encoding = argv[3];
    if (encoding == "array") {
      CFG<JaggedArrayInt>* cfg = loadGrammar<JaggedArrayInt>(type, filename);
      benchmark(cfg, querySize, numQueries, eng);
      delete cfg;
    } else if (encoding == "bpleft") {
      CFG<JaggedArrayBpIndex>* cfg = loadGrammar<JaggedArrayBpIndex>(type, filename);
      benchmark(cfg, querySize, numQueries, eng);
      delete cfg;
    } else if (encoding == "bpright") {
      CFG<JaggedArrayBpOpt>* cfg = loadGrammar<JaggedArrayBpOpt>(type, filename);
      benchmark(cfg, querySize, numQueries, eng);
      delete cfg;
    } else if (encoding == "bpmono") {
      CFG<JaggedArrayBpMono>* cfg = loadGrammar<JaggedArrayBpMono>(type, filename);
      benchmark(cfg, querySize, numQueries, eng);
      delete cfg;
    } else {
      cerr << "invalid grammar encoding: \"" << encoding << "\"" << endl;
    }

    return 1;
}
