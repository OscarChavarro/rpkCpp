#ifndef __COMMON_LOOKUP_ENTITY__
#define __COMMON_LOOKUP_ENTITY__

#include "vsdk/toolkit/common/VSDK.h"

template<typename T>
class LookUpEntity {
  public:
    LookUpEntity():
        key(NULL),
        value(0),
        data()
    {
    }

    char *key; // Key name
    long value; // Key hash value (for efficiency)
    T data; // Client data
};

#endif
