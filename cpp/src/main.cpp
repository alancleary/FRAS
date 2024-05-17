#include <iostream>
#include <chrono>
#include <random>
//#include "cfg-amt/cfg.hpp"
#include "cfg-amt/compressed-indexed-cfg.hpp"
//#include "cfg-amt/indexed-cfg.hpp"

#include "cfg-amt/amt/key.hpp"

using namespace std;
using namespace cfg_amt;

void usage(int argc, char* argv[]) {
    cerr << "usage: " << argv[0] << " <type> <filename>" << endl;
    cerr << endl;
    cerr << "args: " << endl;
    cerr << "\ttype={mrrepair|navarro|bigrepair}: the type of grammar to load" << endl;
    cerr << "\t\tmrrepair: for grammars created with the MR-RePair algorithm" << endl;
    cerr << "\t\tnavarro: for grammars created with Navarro's implementation of RePair" << endl;
    cerr << "\t\tbigrepair: for grammars created with Manzini's implementation of Big-Repair" << endl;
    cerr << "\tfilename: the name of the grammar file(s) without the extension(s)" << endl;
}

CompressedIndexedCFG* loadGrammar(string type, string filename) {
    if (type == "mrrepair") {
        //return IndexedCFG::fromMrRepairFile(filename + ".out");
        return CompressedIndexedCFG::fromMrRepairFile(filename + ".out");
    } else if (type == "navarro") {
        //return IndexedCFG::fromNavarroFiles(filename + ".C", filename + ".R");
        return CompressedIndexedCFG::fromNavarroFiles(filename + ".C", filename + ".R");
    } else if (type == "bigrepair") {
        //return IndexedCFG::fromBigRepairFiles(filename + ".C", filename + ".R");
        return CompressedIndexedCFG::fromBigRepairFiles(filename + ".C", filename + ".R");
    }
    cerr << "invalid grammar type: \"" << type << "\"" << endl;
    cerr << endl;
    return NULL;
}

int main(int argc, char* argv[])
{

    // check the command-line arguments
    if (argc < 3) {
      usage(argc, argv);
      return 1;
    }

    // load the grammar
    string type = argv[1];
    string filename = argv[2];
    //IndexedCFG* cfg = loadGrammar(type, filename);
    CompressedIndexedCFG* cfg = loadGrammar(type, filename);
    cerr << "text length: " << cfg->getTextLength() << endl;
    cerr << "num rules: " << cfg->getNumRules() << endl;
    cerr << "start size: " << cfg->getStartSize() << endl;
    cerr << "rules size: " << cfg->getRulesSize() << endl;
    cerr << "total size: " << cfg->getTotalSize() << endl;
    cerr << "depth: " << cfg->getDepth() << endl;
    /*
    cerr << endl;
    cerr << "map entries: " << cfg->getNumMapEntries() << endl;
    cerr << "map size: " << cfg->getMapSize() << endl;
    cerr << endl;
    auto tailInfo = cfg->tailInfo();
    cerr << "map num tails: " << tailInfo.first << endl;
    cerr << "map total tail nodes: " << tailInfo.second << endl;
    cerr << endl;
    */
    
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

    //cout.setstate(std::ios::failbit);
    for (int i = 0; i < numQueries; i++) {
        begin = distr(gen);
        end = begin + querySize - 1;

        startTime = chrono::steady_clock::now();
        cfg->get(cout, begin, end);
        endTime = chrono::steady_clock::now();
        duration += chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();
    }

    cerr << "average query time: " << duration / numQueries << "[µs]" << endl;

    //delete cfg;

    return 1;
}
