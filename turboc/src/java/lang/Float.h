#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __JAVA_FLOAT__
#define __JAVA_FLOAT__

#include "common/VSDK.h"

class Float {
  public:
    #define FLOAT_MIN_VALUE ((float)1.40129846e-45)
    #define FLOAT_MAX_VALUE ((float)3.40282347e+38)

    static bool isFinite(float a);
};


#endif
