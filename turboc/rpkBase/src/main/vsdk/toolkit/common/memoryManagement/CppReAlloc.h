#ifndef __CPP_RE_ALLOC__
#define __CPP_RE_ALLOC__

#include "vsdk/toolkit/common/VSDK.h"

class CppReAlloc {
  public:
    CppReAlloc();

    static unsigned char *reAlloc(
        unsigned char *ptr,
        int oldElementCount,
        int newElementCount);

    static char **reAlloc(
        char **ptr,
        int oldElementCount,
        int newElementCount);
};

#endif
