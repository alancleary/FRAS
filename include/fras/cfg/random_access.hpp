#ifndef INCLUDED_FRAS_CFG_RANDOM_ACCESS
#define INCLUDED_FRAS_CFG_RANDOM_ACCESS

#include <cstdint>
#include <iterator>
#include <ostream>
#include <stack>

namespace fras {

/** An abstract class that adds random access support to a CFG. */
template <class CFG_T>
class RandomAccess
{
    private:
        std::stack<int> ruleStack;
        std::stack<int> indexStack;

        virtual void rankSelect(uint64_t i, int& rank, uint64_t& select) = 0;
        virtual uint64_t expansionSize(int rule) = 0;

    public:

        CFG_T* cfg;

        RandomAccess(CFG_T* cfg): cfg(cfg), ruleStack(), indexStack() { };

        /**
          * Gets a substring in the original string.
          *
          * @param out The output stream to write the substring to.
          * @param begin The start position of the substring in the original string.
          * @param end The end position of the substring in the original string.
          * @throws Exception if begin or end is out of bounds.
          */
        //void get(std::ostream& out, uint64_t begin, uint64_t end);
        void get(char* out, uint64_t begin, uint64_t end)
        {
            //if (begin < 0 || end >= cfg->textLength || begin > end) {
            //    throw std::runtime_error("begin/end out of bounds");
            //}
            uint64_t length = end - begin;

            // get the start rule character to start parsing at
            int c, rank, r = cfg->getStartRule();
            uint64_t selected;
            rankSelect(begin, rank, selected);
            int i = rank - 1;

            // descend the parse tree to the correct start position
            uint64_t size, ignore = begin - selected;
            // TODO: stacks should be preallocated to size of max depth
            while (ignore > 0) {
                c = cfg->get(r, i);
                // terminal character
                if (c < CFG_T::ALPHABET_SIZE) {
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
                c = cfg->get(r, i);
                if (c == CFG_T::DUMMY_CODE) {
                    r = ruleStack.top();
                    ruleStack.pop();
                    i = indexStack.top();
                    indexStack.pop();
                // terminal character
                } else if (c < CFG_T::ALPHABET_SIZE) {
                    //out << (char) c;
                    out[j] = (char) c;
                    i++;
                    j++;
                // non-terminal character
                } else {
                    ruleStack.push(r);
                    r = c;
                    indexStack.push(i + 1);
                    i = 0;
                }
            }
        }

        class Iterator;

        Iterator begin() { return Iterator(this, 0); };
        Iterator begin(int pos) { return Iterator(this, pos); };
        Iterator end() { return Iterator(this, this->cfg->getTextLength()); };
};


/** An iterator for iterating the text in the CFG. */
template <class CFG_T>
class RandomAccess<CFG_T>::Iterator
{

using iterator_category = std::forward_iterator_tag;
//using difference_type = std::ptrdiff_t;
using value_type = char;
using pointer = char*;
using reference = char&;

private:

    //const CFG* parent;
    const RandomAccess<CFG_T>* ra;

    // TODO: stacks should be preallocated to size of max depth
    //std::stack<int> ruleStack;
    //std::stack<int> indexStack;
    //int skip;  // how many characters to skip before decoding
    //int r;  // current rule being decoded
    //int i;  // index in r of current (non-)terminal being decoded
    int j;  // currently decoded character in text

    value_type m_char;

    void next();

public:

    //Iterator(const CFG* cfg, int pos);
    Iterator(const RandomAccess<CFG_T>* ra, int pos) : ra(ra), j(pos)
    {
        // return the end iterator if out of bounds
        if (pos < 0 || pos >= ra->cfg->getTextLength()) {
            j = ra->cfg->getTextLength();
            return;
        }

        next();
    }

    // dereference
    const reference operator*() { return m_char; };
    const pointer operator->() { return &m_char; };

    // prefix increment
    Iterator& operator++()
    {
        j++;
        next();
        return *this;
    }

    // postfix increment
    Iterator operator++(int)
    {
        Iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    // comparators
    bool operator==(const Iterator& itr) { return this->j == itr.j; };
    bool operator!=(const Iterator& itr) { return this->j != itr.j; };

};

}

#endif
