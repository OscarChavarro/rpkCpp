#include "java/lang/Float.h"
#include "common/linealAlgebra/Numeric.h"

const double Numeric::HUGE_DOUBLE_VALUE = 1e30;
const float Numeric::HUGE_FLOAT_VALUE = java::Float::MAX_VALUE;
const double Numeric::EPSILON = 1e-6;
const float Numeric::EPSILON_FLOAT = 1e-6F;

/**
Returns whether the first floating point value is greater than the second one.
*/
bool
Numeric::floatCompare(const float x, const float y) {
    return x > y;
}
