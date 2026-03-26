#ifndef __BINARY_MODEL_READER__
#define __BINARY_MODEL_READER__

class MgfModel;

class BinaryModelReader {
  public:
    static MgfModel *read(const char *fileName);
};

#endif
