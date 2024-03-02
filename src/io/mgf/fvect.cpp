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
        len = sqrt(len);
    }

    v[0] /= len;
    v[1] /= len;
    v[2] /= len;

    return len;
}

/**
Cross product of two vectors
vres = v1 X v2
*/
void
floatCrossProduct(FVECT vres, FVECT v1, FVECT v2)
{
    vres[0] = v1[1] * v2[2] - v1[2] * v2[1];
    vres[1] = v1[2] * v2[0] - v1[0] * v2[2];
    vres[2] = v1[0] * v2[1] - v1[1] * v2[0];
}
