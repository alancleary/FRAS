#ifndef INCLUDED_CFG_RANDOM_ACCESS_V2
#define INCLUDED_CFG_RANDOM_ACCESS_V2

#include <ostream>
#include <stack>
#include "cfg/cfg.hpp"

namespace cfg {

/** An abstract class that adds random access support to a CFG. */
class RandomAccessV2
{
    private:
        std::stack<int> ruleStack;
        std::stack<int> indexStack;

        virtual void rankSelect(uint64_t i, int& rank, uint64_t& select) = 0;
        virtual uint64_t expansionSize(int rule) = 0;

    protected:

        CFG* cfg;

    public:

        RandomAccessV2(CFG* cfg): cfg(cfg), ruleStack(), indexStack() { };

        /**
          * Gets a substring in the original string.
          *
          * @param out The output stream to write the substring to.
          * @param begin The start position of the substring in the original string.
          * @param end The end position of the substring in the original string.
          * @throws Exception if begin or end is out of bounds.
          */
        //void get(std::ostream& out, uint64_t begin, uint64_t end);
        void get(char* out, uint64_t begin, uint64_t end);
};

}

#endif
