#ifndef __MYMATH__
#define __MYMATH__

#include <cmath>

#define QSORT_CALLBACK_TYPE int (*)(const void *, const void *)

#ifndef HUGE
    #define HUGE 1e30
#endif

extern int fcmp(const float *x, const float *y);

#endif
