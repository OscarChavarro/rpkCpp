#include "common/linealAlgebra/Numeric.h"

#include <cfloat>

const double Numeric::HUGE_DOUBLE_VALUE = 1e30;
const float Numeric::HUGE_FLOAT_VALUE = FLT_MAX;
const double Numeric::EPSILON = 1e-6;
const float Numeric::EPSILON_FLOAT = 1e-6f;

/**
Compares two double floating point values pointed to by v1 and v2
*/
int
Numeric::floatCompare(const float *x, const float *y) {
    if ( x > y ) {
        return 1;
    } else {
        return (x < y) ? -1 : 0;
    }
}
