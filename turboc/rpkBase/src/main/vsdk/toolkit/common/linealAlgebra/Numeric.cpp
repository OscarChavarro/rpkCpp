#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"

const double Numeric::HUGE_DOUBLE_VALUE = 1e30;
const float Numeric::EPSILON_FLOAT = 1e-6f;

/**
Returns whether the first floating point value is greater than the second one.
*/
bool
Numeric::floatCompare(const float x, const float y) {
    return x > y;
}
