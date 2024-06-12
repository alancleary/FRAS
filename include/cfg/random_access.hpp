#ifndef INCLUDED_CFG_RANDOM_ACCESS
#define INCLUDED_CFG_RANDOM_ACCESS

#include <ostream>
#include "cfg/cfg.hpp"

namespace cfg {

/** An abstract class that adds random access support to a CFG. */
class RandomAccess
{
    private:

        virtual void rankSelect(uint64_t i, int& rank, uint64_t& select) = 0;

    protected:

        CFG* cfg;

    public:

        RandomAccess(CFG* cfg): cfg(cfg) { };

        /**
          * Gets a substring in the original string.
          *
          * @param out The output stream to write the substring to.
          * @param begin The start position of the substring in the original string.
          * @param end The end position of the substring in the original string.
          * @throws Exception if begin or end is out of bounds.
          */
        void get(std::ostream& out, uint64_t begin, uint64_t end);
};

}

#endif
