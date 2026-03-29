#ifndef __BINARY_MODEL_READER_INDEX_LIST_RECORD__
#define __BINARY_MODEL_READER_INDEX_LIST_RECORD__

#include "java/util/ArrayList.h"

#include "io/bin/BinaryModelReader.h"

class BinaryModelReader::IndexListRecord {
  public:
    bool isNull;
    java::ArrayList<int> *indices;

    IndexListRecord():
        isNull(true),
        indices(nullptr)
    {
    }
};

#endif
