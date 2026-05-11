#ifndef BINARY_MODEL_READER_INDEX_LIST_RECORD__
#define BINARY_MODEL_READER_INDEX_LIST_RECORD__

#include "vsdk/toolkit/java/util/ArrayList.h"

class BinaryModelIndexListRef {
  public:
    bool isNull;
    java::ArrayList<int> *indices;

    BinaryModelIndexListRef();
};

#endif
