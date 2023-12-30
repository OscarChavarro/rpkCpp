#ifndef _MYMATH_H_INCLUDED_
#define _MYMATH_H_INCLUDED_

#include <cmath>

//#define QSORT_CALLBACK_TYPE __compar_fn_t
#define QSORT_CALLBACK_TYPE int (*)(const void *, const void *)

#ifndef HUGE
    #define HUGE 1e30
#endif

extern int fcmp(const float *x, const float *y);

#endif
