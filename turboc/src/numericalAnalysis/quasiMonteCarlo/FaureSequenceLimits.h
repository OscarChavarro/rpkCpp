#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __FAURE_SEQUENCE_LIMITS__
#define __FAURE_SEQUENCE_LIMITS__

#include "common/VSDK.h"

class FaureSequenceLimits {
  public:
    enum{
        MAX_DIMENSION = 10,
        MAX_PRIME_DIGITS = 30,
        MAX_SEED = 2147483647
    };
};

#endif
