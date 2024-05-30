#include <stack>
#include <stdexcept>
#include "cfg/random_access.hpp"

namespace cfg {

// random access

void RandomAccess::get(std::ostream& out, uint64_t begin, uint64_t end)
{
    if (begin < 0 || end >= cfg->textLength || begin > end) {
        throw std::runtime_error("begin/end out of bounds");
    }

    int i, r = cfg->startRule;
    uint64_t selected;
    rankSelect(begin, i, selected);
    uint64_t length = end - selected;
    uint64_t ignore = begin - selected;
    // TODO: stacks should be preallocated to size of max depth
    std::stack<int> ruleStack;
    std::stack<int> indexStack;
    for (uint64_t j = 0; j < length;) {
        // end of rule
        if (cfg->rules[r][i] == CFG::DUMMY_CODE) {
            r = ruleStack.top();
            ruleStack.pop();
            i = indexStack.top();
            indexStack.pop();
        // terminal character 
        } else if (cfg->rules[r][i] < CFG::ALPHABET_SIZE) {
            if (ignore > 0) {
                ignore--;
            } else {
                out << (char) cfg->rules[r][i];
            }
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
