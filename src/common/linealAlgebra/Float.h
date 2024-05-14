#ifndef __FLOAT__
#define __FLOAT__

#include <cmath>
#include <cfloat>

#define QSORT_CALLBACK_TYPE int (*)(const void *, const void *)

extern double HUGE_DOUBLE_VALUE;
extern float HUGE_FLOAT_VALUE;

extern const double EPSILON;
extern const float EPSILON_FLOAT;

/**
Tests whether two floating point numbers are equal within the given tolerance
*/
inline bool
doubleEqual(double a, double b, double tolerance) {
    return (a - b) > -tolerance && (a - b) < tolerance;
}

extern int floatCompare(const float *x, const float *y);

#endif
