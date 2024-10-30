#ifndef INCLUDED_FRAS_ARRAY_JAGGED_ARRAY_INT
#define INCLUDED_FRAS_ARRAY_JAGGED_ARRAY_INT

#include <cstddef>
#include <cstdlib>
#include <malloc.h>
#include <new>
#include "fras/array/jagged_array.hpp"

namespace fras {

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

    int getMemSize() {
      int memSize = 0;
      for (int i = 0; i < numArrays; i++) {
        int* array = arrays[i];
        if (array == NULL) continue;
        int j = 0;
        while (true) {
          uint8_t value = array[j++];
          if (value == 0) break;
        }
        memSize += sizeof(int) * j;
      }
      return memSize;
    }

    void setArray(int index, int* array, int length)
    {
      arrays[index] = (int*) realloc(arrays[index], sizeof(int) * length);
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
