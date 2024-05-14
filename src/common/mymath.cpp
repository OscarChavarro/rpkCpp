#include "common/mymath.h"

double HUGE_DOUBLE_VALUE = 1e30;
float HUGE_FLOAT_VALUE = FLT_MAX;

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
