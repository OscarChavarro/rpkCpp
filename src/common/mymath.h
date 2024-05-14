#ifndef __MY_MATH__
#define __MY_MATH__

#include <cmath>
#include <cfloat>

#define QSORT_CALLBACK_TYPE int (*)(const void *, const void *)

extern double HUGE_DOUBLE_VALUE;
extern float HUGE_FLOAT_VALUE;

extern int floatCompare(const float *x, const float *y);

#endif
