#ifndef __BINARY_MODEL_READER_INDEX_LIST_RECORD__
#define __BINARY_MODEL_READER_INDEX_LIST_RECORD__

#include "java/util/ArrayList.h"

class BinaryModelIndexListRef {
  public:
    bool isNull;
    java::ArrayList<int> *indices;

    BinaryModelIndexListRef();
};

#endif
