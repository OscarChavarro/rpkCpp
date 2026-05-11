#ifndef BINARY_MODEL_READER__
#define BINARY_MODEL_READER__

#include "io/context/ParseSnapshotContext.h"

class BinaryModelDeserializer {
  public:
    static ParseSnapshotContext *read(const char *fileName);
};

#endif
