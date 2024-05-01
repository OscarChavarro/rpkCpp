#include "common/mymath.h"

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
