/**
Routines for 3-d vectors
*/

#include "java/lang/Math.h"
#include "common/linealAlgebra/Vector3Dd.h"

/**
Normalize a vector, return old magnitude
*/
double
normalize(double *v, double epsilon)
{
    static double len;

    len = dotProduct(v, v);

    if ( len <= 0.0 ) {
        return 0.0;
    }

    if ( len <= 1.0 + epsilon && len >= 1.0 - epsilon) {
        // First order approximation
        len = 0.5 + 0.5 * len;
    } else {
        len = java::Math::sqrt(len);
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
floatCrossProduct(VECTOR3Dd result, const VECTOR3Dd a, const VECTOR3Dd b)
{
    result[0] = a[1] * b[2] - a[2] * b[1];
    result[1] = a[2] * b[0] - a[0] * b[2];
    result[2] = a[0] * b[1] - a[1] * b[0];
}

/**
Returns squared distance between the two vectors
*/
double
distanceSquared(VECTOR3Dd *v1, VECTOR3Dd *v2) {
    VECTOR3Dd d;

    d[0] = (*v2)[0] - (*v1)[0];
    d[1] = (*v2)[1] - (*v1)[1];
    d[2] = (*v2)[2] - (*v1)[2];
    return d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
}
