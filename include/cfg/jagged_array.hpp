#ifndef INCLUDED_CFG_JAGGED_ARRAY
#define INCLUDED_CFG_JAGGED_ARRAY

namespace cfg {

/** An abstract class that provides an interface for jagged array implementations. */
class JaggedArray
{
  protected:

    int numArrays;

  public:

    JaggedArray(int numArrays): numArrays(numArrays) { }

    virtual void setArray(int index, int* array, int length) = 0;
    virtual void clearArray(int index) = 0;
    virtual int getValue(int index, int item) = 0;

    const int& getNumArrays() const { return numArrays; }
    virtual int getMemSize() = 0;
};

}

#endif
