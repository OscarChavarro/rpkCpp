#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __SGL_CONSTANTS__
#define __SGL_CONSTANTS__

#include "common/VSDK.h"

class SglConstants{ public:
    static const unsigned long SGL_MAXIMUM_Z = 4294967295UL;
    static const int SGL_TRANSFORM_STACK_SIZE = 4;
};

#endif
