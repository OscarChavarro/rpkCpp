#ifndef __BINARY_MODEL_READER__
#define __BINARY_MODEL_READER__

#include "io/context/PersistedSceneModel.h"

class BinaryModelReader {
  public:
    static PersistedSceneModel *read(const char *fileName);
};

#endif
