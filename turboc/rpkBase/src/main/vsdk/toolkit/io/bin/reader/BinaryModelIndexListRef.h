#ifndef __BINARY_MODEL_INDEX_LIST_REF__
#define __BINARY_MODEL_INDEX_LIST_REF__

#include "java/util/ArrayList.h"

class BinaryModelIndexListRef {
  public:
    bool isNull;
    ArrayList<int> *indices;

    BinaryModelIndexListRef();
};

#endif
