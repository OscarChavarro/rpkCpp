#ifndef _RPK_FLOAT_H_
#define _RPK_FLOAT_H_

#include <cmath>

#define EPSILON 1e-6

/**
Tests whether two floating point numbers are equal within the given tolerance
*/
#define floatEqual(a, b, tolerance) (((a)-(b)) > -(tolerance) && ((a)-(b)) < (tolerance))

#ifndef MAX
    #define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
    #define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

// Yes, float or double makes a difference!
#define REAL double

inline REAL
ABS(REAL A) {
    return A < 0.0 ? -A : A;
}

// Common to vector types
#define X_NORMAL 0
#define Y_NORMAL 1
#define Z_NORMAL 2
#define X_GREATER 0x01
#define Y_GREATER 0x02
#define Z_GREATER 0x04
#define XYZ_EQUAL 0x08

#endif
