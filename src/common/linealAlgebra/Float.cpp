#include "common/linealAlgebra/Float.h"

const double HUGE_DOUBLE_VALUE = 1e30;
const float HUGE_FLOAT_VALUE = FLT_MAX;

const double EPSILON = 1e-6;
const float EPSILON_FLOAT = 1e-6f;

/**
Compares two double floating point values pointed to by v1 and v2
*/
int
floatCompare(const float *x, const float *y) {
    if ( x > y ) {
        return 1;
    } else {
        return (x < y) ? -1 : 0;
    }
}
