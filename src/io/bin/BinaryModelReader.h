#ifndef __BINARY_MODEL_READER__
#define __BINARY_MODEL_READER__

class PersistedSceneModel;

class BinaryModelReader {
  public:
    static PersistedSceneModel *read(const char *fileName);
};

#endif
