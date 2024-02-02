#include <iostream>
#include <chrono>
#include <random>
#include "cfg-amt/cfg.hpp"

using namespace std;
using namespace cfg_amt;


void usage(int argc, char* argv[]) {
    cerr << "Usage: " << argv[0] << " <MR_REPAIR_CFG>" << endl;
}

int main(int argc, char* argv[])
{

    // check the command-line arguments
    if (argc < 2) {
      usage(argc, argv);
      return 1;
    }

    // load the grammar
    string filepath = argv[1];
    CFG* cfg = CFG::fromMrRepairFile(filepath);
    cerr << "text length: " << cfg->getTextLength() << endl;
    cerr << endl;
    cerr << "num rules: " << cfg->getNumRules() << endl;
    cerr << "start size: " << cfg->getStartSize() << endl;
    cerr << "rules size: " << cfg->getRulesSize() << endl;
    cerr << "total size: " << cfg->getTotalSize() << endl;
    //cerr << "depth: " << cfg->getDepth() << endl;
    cerr << endl;
    cerr << "map entries: " << cfg->getNumMapEntries() << endl;
    cerr << endl;
    
    // generate the original text
    //cfg->get(cout, 0, cfg->getTextLength() - 1);

    // benchmarks
    chrono::steady_clock::time_point startTime, endTime;
    uint64_t duration = 0;
    int numQueries = 10000, querySize = 1000;
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<uint32_t> distr(0, cfg->getTextLength() - querySize);
    uint32_t begin, end;

    for (int i = 0; i < numQueries; i++) {
        begin = distr(gen);
        end = begin + querySize - 1;

        startTime = chrono::steady_clock::now();
        cfg->get(cout, begin, end);
        endTime = chrono::steady_clock::now();
        duration += chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();
    }

    cerr << "average query time: " << duration / numQueries << "[µs]" << endl;

    return 1;
}
