#include <iostream>
#include <chrono>
#include <random>
//#include "cfg-amt/cfg.hpp"
#include "cfg-amt/compressed-indexed-cfg.hpp"
#include "cfg-amt/compressed-indexed-cfg_v2.hpp"
#include "cfg-amt/bitvector-indexed-cfg.hpp"
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
        return CompressedIndexedCFG::fromMrRepairFile(filename + ".out");
    } else if (type == "navarro") {
        return CompressedIndexedCFG::fromNavarroFiles(filename + ".C", filename + ".R");
    } else if (type == "bigrepair") {
        return CompressedIndexedCFG::fromBigRepairFiles(filename + ".C", filename + ".R");
    }
    cerr << "invalid grammar type: \"" << type << "\"" << endl;
    cerr << endl;
    return NULL;
}

CompressedIndexedCFGV2* loadGrammarV2(string type, string filename) {
    if (type == "mrrepair") {
        return CompressedIndexedCFGV2::fromMrRepairFile(filename + ".out");
    } else if (type == "navarro") {
        return CompressedIndexedCFGV2::fromNavarroFiles(filename + ".C", filename + ".R");
    } else if (type == "bigrepair") {
        return CompressedIndexedCFGV2::fromBigRepairFiles(filename + ".C", filename + ".R");
    }
    cerr << "invalid grammar type: \"" << type << "\"" << endl;
    cerr << endl;
    return NULL;
}

BitvectorIndexedCFG* loadBitvectorGrammar(string type, string filename) {
    if (type == "mrrepair") {
        return BitvectorIndexedCFG::fromMrRepairFile(filename + ".out");
    } else if (type == "navarro") {
        return BitvectorIndexedCFG::fromNavarroFiles(filename + ".C", filename + ".R");
    } else if (type == "bigrepair") {
        return BitvectorIndexedCFG::fromBigRepairFiles(filename + ".C", filename + ".R");
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

    CompressedIndexedCFG* cfg = loadGrammar(type, filename);
    cerr << "text length: " << cfg->getTextLength() << endl;
    cerr << "num rules: " << cfg->getNumRules() << endl;
    cerr << "start size: " << cfg->getStartSize() << endl;
    cerr << "rules size: " << cfg->getRulesSize() << endl;
    cerr << "total size: " << cfg->getTotalSize() << endl;
    cerr << "depth: " << cfg->getDepth() << endl;
    //std::cerr << std::endl;
    //cfg->tmp();
    std::cerr << std::endl;

    CompressedIndexedCFGV2* cfgV2 = loadGrammarV2(type, filename);
    cerr << "text length: " << cfgV2->getTextLength() << endl;
    cerr << "num rules: " << cfgV2->getNumRules() << endl;
    cerr << "start size: " << cfgV2->getStartSize() << endl;
    cerr << "rules size: " << cfgV2->getRulesSize() << endl;
    cerr << "total size: " << cfgV2->getTotalSize() << endl;
    cerr << "depth: " << cfgV2->getDepth() << endl;
    //std::cerr << std::endl;
    //cfgV2->tmp();
    std::cerr << std::endl;

    BitvectorIndexedCFG* cfgBV = loadBitvectorGrammar(type, filename);
    cerr << "text length: " << cfgBV->getTextLength() << endl;
    cerr << "num rules: " << cfgBV->getNumRules() << endl;
    cerr << "start size: " << cfgBV->getStartSize() << endl;
    cerr << "rules size: " << cfgBV->getRulesSize() << endl;
    cerr << "total size: " << cfgBV->getTotalSize() << endl;
    cerr << "depth: " << cfgBV->getDepth() << endl;
    cerr << "memory: " << cfgBV->getMemSize() << " bits" << endl;
    cerr << "memory v5: " << cfgBV->getMemSizeV5() << " bits" << endl;
    cerr << "memory il: " << cfgBV->getMemSizeIl() << " bits" << endl;
    //cerr << "memory rrr: " << cfgBV->getMemSizeRRR() << " bits" << endl;
    cerr << "memory sparse: " << cfgBV->getMemSizeSparse() << " bits" << endl;
    std::cerr << std::endl;

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
    uint64_t durationV2 = 0;
    uint64_t bitvectorDuration = 0;
    int numQueries = 10000, querySize = 1000;
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<uint32_t> distr(0, cfg->getTextLength() - querySize);
    uint32_t begin, end;

    cout.setstate(std::ios::failbit);
    for (int i = 0; i < numQueries; i++) {
        begin = distr(gen);
        end = begin + querySize - 1;

        // AMT query
        startTime = chrono::steady_clock::now();
        cfg->get(cout, begin, end);
        endTime = chrono::steady_clock::now();
        duration += chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();

        // AMTv2 query
        startTime = chrono::steady_clock::now();
        cfgV2->get(cout, begin, end);
        endTime = chrono::steady_clock::now();
        durationV2 += chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();

        // bitvector query
        startTime = chrono::steady_clock::now();
        cfgBV->get(cout, begin, end);
        endTime = chrono::steady_clock::now();
        bitvectorDuration += chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();

        begin = distr(gen);
        end = begin + querySize - 1;
    }

    cerr << "average AMT query time: " << duration / numQueries << "[µs]" << endl;

    cerr << "average AMT V2 query time: " << durationV2 / numQueries << "[µs]" << endl;

    cerr << "average bitvector query time: " << bitvectorDuration / numQueries << "[µs]" << endl;

    //delete cfg;

    return 1;
}
