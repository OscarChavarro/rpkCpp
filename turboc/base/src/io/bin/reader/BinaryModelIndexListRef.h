#ifndef BNRY_MDL_RDR_INDX_LIST_RCRD
#define BNRY_MDL_RDR_INDX_LIST_RCRD

#include "java/util/ArrayList.h"

class BinaryModelIndexListRef {
  public:
    bool isNull;
    ArrayList<int> *indices;

    BinaryModelIndexListRef();
};

#endif
