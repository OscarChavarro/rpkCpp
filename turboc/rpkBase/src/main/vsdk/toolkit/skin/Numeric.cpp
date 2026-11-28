#include "vsdk/toolkit/skin/Numeric.h"

const double Numeric::HUGE_DOUBLE_VALUE = 1e30;
const double Numeric::EPSILON = 1e-6;

/**
Returns whether the first floating point value is greater than the second one.
*/
bool
Numeric::floatCompare(const double x, const double y) {
    return x > y;
}
