#ifndef _RPK_FLOAT_H_
#define _RPK_FLOAT_H_

#include <cmath>

#define EPSILON 1e-6

/* tests whether two floating point numbers are eual within the given tolerance */
#define FLOATEQUAL(a, b, tolerance) (((a)-(b)) > -(tolerance) && ((a)-(b)) < (tolerance))

#ifndef MAX
    #define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
    #define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

// Yes, float or double makes a difference!
#define REAL double

#endif
