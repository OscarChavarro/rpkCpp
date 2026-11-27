#ifndef __LOOK_UP_ENTITY__
#define __LOOK_UP_ENTITY__

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
