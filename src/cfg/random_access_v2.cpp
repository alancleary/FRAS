//#include <stack>
#include <stdexcept>
#include "cfg/cfg.hpp"
#include "cfg/jagged_array_int.hpp"
#include "cfg/random_access_v2.hpp"

namespace cfg {

// random access

//void RandomAccessV2::get(std::ostream& out, uint64_t begin, uint64_t end)
template <class CFG_T>
void RandomAccessV2<CFG_T>::get(char* out, uint64_t begin, uint64_t end)
{
    //if (begin < 0 || end >= cfg->textLength || begin > end) {
    //    throw std::runtime_error("begin/end out of bounds");
    //}
    uint64_t length = end - begin;

    // get the start rule character to start parsing at
    int rank, r = cfg->getStartRule();
    uint64_t selected;
    rankSelect(begin, rank, selected);
    int i = rank - 1;

    // descend the parse tree to the correct start position
    uint64_t size, ignore = begin - selected;
    // TODO: stacks should be preallocated to size of max depth
    while (ignore > 0) {
        // terminal character 
        if (cfg->get(r, i) < CFG_T::ALPHABET_SIZE) {
            i++;
            ignore--;
        // non-terminal character
        } else {
            size = expansionSize(cfg->get(r, i));
            if (size > ignore) {
                ruleStack.push(r);
                r = cfg->get(r, i);
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
        if (cfg->get(r, i) == CFG_T::DUMMY_CODE) {
            r = ruleStack.top();
            ruleStack.pop();
            i = indexStack.top();
            indexStack.pop();
        // terminal character 
        } else if (cfg->get(r, i) < CFG_T::ALPHABET_SIZE) {
            //out << (char) cfg->get(r, i);
            out[j] = (char) cfg->get(r, i);
            i++;
            j++;
        // non-terminal character
        } else {
            ruleStack.push(r);
            r = cfg->get(r, i);
            indexStack.push(i + 1);
            i = 0;
        }
    }
}

// instantiate the class
template class RandomAccessV2<CFG<JaggedArrayInt>>;

}
