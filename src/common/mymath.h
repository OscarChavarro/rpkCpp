#ifndef __MY_MATH__
#define __MY_MATH__

#include <cmath>
#include <cfloat>

#define QSORT_CALLBACK_TYPE int (*)(const void *, const void *)

#ifndef HUGE
    #define HUGE 1e30
#endif

#ifndef HUGE_FLOAT
    #define HUGE_FLOAT FLT_MAX
#endif

extern int floatCompare(const float *x, const float *y);

#endif
