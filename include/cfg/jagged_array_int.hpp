#ifndef INCLUDED_CFG_JAGGED_ARRAY_INT
#define INCLUDED_CFG_JAGGED_ARRAY_INT

#include <cstddef>
#include <cstdlib>
#include <malloc.h>
#include <new>
#include "cfg/jagged_array.hpp"

namespace cfg {

/** Implements the jagged array abstract class using int arrays. */
class JaggedArrayInt : public JaggedArray
{
  private:

    int** arrays;

  public:

    JaggedArrayInt(int numArrays): JaggedArray(numArrays)
    {
      // initialize the jagged array
      arrays = new int*[numArrays];
      for (int i = 0; i < numArrays; i++) {
        arrays[i] = NULL;
      }
    };

    ~JaggedArrayInt()
    {
      for (int i = 0; i < numArrays; i++) {
        free(arrays[i]);
      }
      delete[] arrays;
    }

    void setArray(int index, int* array, int length)
    {
      memSize -= malloc_usable_size(arrays[index]);
      arrays[index] = (int*) realloc(arrays[index], sizeof(int) * length);
      memSize += malloc_usable_size(arrays[index]);
      if (arrays[index] == NULL) {
        throw std::bad_alloc();
      }
      for (int i = 0; i < length; i++) {
        arrays[index][i] = array[i];
      }
    }

    void clearArray(int index)
    {
      free(arrays[index]);
      arrays[index] = NULL;
    }

    int getValue(int index, int item)
    {
      return arrays[index][item];
    }
};

}

#endif
