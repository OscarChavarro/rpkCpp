#ifndef __BINARY_MODEL_READER__
#define __BINARY_MODEL_READER__

#include "io/context/ParseSnapshotContext.h"

class BinaryModelReader {
  public:
    static ParseSnapshotContext *read(const char *fileName);
};

#endif
