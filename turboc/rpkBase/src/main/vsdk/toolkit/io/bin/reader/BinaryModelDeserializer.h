#ifndef __BINARY_MODEL_DESERIALIZER__
#define __BINARY_MODEL_DESERIALIZER__

#include "vsdk/toolkit/io/context/ParseSnapshotContext.h"

class BinaryModelDeserializer {
  public:
    static ParseSnapshotContext *read(const char *fileName);
};

#endif
