#include <math.h>
#include <float.h>

#include "java/lang/Float.h"


bool
Float::isFinite(float a) {
#if defined(isfinite)
    return isfinite(a) != 0;
#elif defined(__GNUC__) || defined(__unix__) || defined(__APPLE__)
    return finite(((double)(a))) != 0;
#else
    if ( a != a ) {
        return false;
    }
    return a <= FLT_MAX && a >= -FLT_MAX;
#endif
}
