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
    while (ignore > 0) {
        // terminal character 
        if (cfg->rules[r][i] < CFG::ALPHABET_SIZE) {
            i++;
            ignore--;
        // non-terminal character
        } else {
            size = expansionSize(cfg->rules[r][i]);
            if (size > ignore) {
                ruleStack.push(r);
                r = cfg->rules[r][i];
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
        if (cfg->rules[r][i] == CFG::DUMMY_CODE) {
            r = ruleStack.top();
            ruleStack.pop();
            i = indexStack.top();
            indexStack.pop();
        // terminal character 
        } else if (cfg->rules[r][i] < CFG::ALPHABET_SIZE) {
            //out << (char) cfg->rules[r][i];
            out[j] = (char) cfg->rules[r][i];
            i++;
            j++;
        // non-terminal character
        } else {
            ruleStack.push(r);
            r = cfg->rules[r][i];
            indexStack.push(i + 1);
            i = 0;
        }
    }
}

}
