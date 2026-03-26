#ifndef __BINARY_MODEL_WRITTER__
#define __BINARY_MODEL_WRITTER__

class MgfModel;

class BinaryModelWritter {
  public:
    static bool write(const MgfModel *model, const char *fileName);
};

#endif
