#include "common/mymath.h"

/**
Compares two double floating point values pointed to by v1 and v2
*/
int
fcmp(const float *x, const float *y) {
    return (x > y) ? +1 : (x < y) ? -1 : 0;
}
