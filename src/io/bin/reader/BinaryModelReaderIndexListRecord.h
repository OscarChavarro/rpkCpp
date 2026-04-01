#ifndef __BINARY_MODEL_READER_INDEX_LIST_RECORD__
#define __BINARY_MODEL_READER_INDEX_LIST_RECORD__

#include "java/util/ArrayList.h"

class BinaryModelReaderIndexListRecord {
  public:
    bool isNull;
    java::ArrayList<int> *indices;

    BinaryModelReaderIndexListRecord();
};

#endif
