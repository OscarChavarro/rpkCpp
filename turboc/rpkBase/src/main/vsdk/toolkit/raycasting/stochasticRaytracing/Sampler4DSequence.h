#ifndef __SAMPLER_4_D_SEQUENCE__
#define __SAMPLER_4_D_SEQUENCE__

#include "vsdk/toolkit/common/VSDK.h"

enum Sampler4DSequence {
    RANDOM,
    HALTON,
    SCRAMBLED_HALTON,
    SOBOL,
    ORIGINAL_FAURE,
    GENERALIZED_FAURE,
    NIEDERREITER
};

#endif
