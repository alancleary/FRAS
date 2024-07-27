//#include <stack>
#include <stdexcept>
#include "cfg/random_access_v2.hpp"

namespace cfg {

// random access

//void RandomAccessV2::get(std::ostream& out, uint64_t begin, uint64_t end)
void RandomAccessV2::get(char* out, uint64_t begin, uint64_t end)
{
    //if (begin < 0 || end >= cfg->textLength || begin > end) {
    //    throw std::runtime_error("begin/end out of bounds");
    //}
    uint64_t length = end - begin;

    // get the start rule character to start parsing at
    int rank, r = cfg->startRule;
    uint64_t selected;
    rankSelect(begin, rank, selected);
    int i = rank - 1;

    // descend the parse tree to the correct start position
    uint64_t size, ignore = begin - selected;
    // TODO: stacks should be preallocated to size of max depth
    //std::stack<int> ruleStack;
    //std::stack<int> indexStack;
<<<<<<< Updated upstream:src/cfg/random_access_v2.cpp
=======


    //in my testing, having to unpack multiple times per process bogs down the speed, so I'm trying to use it as little as possible
    // maybe could do something like store the results of each unpack and save them online for fast access for subsequent runs?
>>>>>>> Stashed changes:cpp/src/cfg/random_access_v2.cpp
    while (ignore > 0) {
        //unpacking rule and storing, used in replacement for all the rules[r][i] bits
        int c = CFG::unpack(r, i);
        // terminal character
        if (c < CFG::ALPHABET_SIZE) {
            i++;
            ignore--;
            // non-terminal character
        } else {
            size = expansionSize(c);
            if (size > ignore) {
                ruleStack.push(r);
                r = c;
                indexStack.push(i + 1);
                i = 0;
            } else {
                ignore -= size;
                i++;
            }
        }
    }

    // decode the substring
    for (uint64_t j = 0; j < length;) {
        // end of rule
        //this checks to see if we are at maximum length for the array (since no dummy codes in packed array)
        if (CFG::ruleLengths[r] == i) {
            r = ruleStack.top();
            ruleStack.pop();
            i = indexStack.top();
            indexStack.pop();
<<<<<<< Updated upstream:src/cfg/random_access_v2.cpp
        // terminal character 
        } else if (cfg->rules[r][i] < CFG::ALPHABET_SIZE) {
            //out << (char) cfg->rules[r][i];
            out[j] = (char) cfg->rules[r][i];
=======
            // terminal character
        } else if (int c = CFG::unpack(r, i) < CFG::ALPHABET_SIZE) {
            out[j] = (char) c;
>>>>>>> Stashed changes:cpp/src/cfg/random_access_v2.cpp
            i++;
            j++;
            // non-terminal character
        } else {
            ruleStack.push(r);
            r = CFG::unpack(r, i);
            indexStack.push(i + 1);
            i = 0;
        }
    }
}

}
