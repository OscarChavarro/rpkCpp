/**
Routines for 3-d vectors
*/

#include "common/mymath.h"
#include "io/mgf/parser.h"

/**
Normalize a vector, return old magnitude
*/
double
normalize(double *v)
{
    static double len;

    len = dotProduct(v, v);

    if ( len <= 0.0 ) {
        return 0.0;
    }

    if ( len <= 1.0 + FLOAT_TINY && len >= 1.0 - FLOAT_TINY) {
	// first order approximation
        len = 0.5 + 0.5 * len;
    } else {
        len = std::sqrt(len);
    }

    v[0] /= len;
    v[1] /= len;
    v[2] /= len;

    return len;
}

/**
Cross product of two vectors
result = a X b
*/
void
floatCrossProduct(FVECT result, const FVECT a, const FVECT b)
{
    result[0] = a[1] * b[2] - a[2] * b[1];
    result[1] = a[2] * b[0] - a[0] * b[2];
    result[2] = a[0] * b[1] - a[1] * b[0];
}
