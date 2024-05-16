/**
Routines for 3-d vectors
*/

#include "java/lang/Math.h"
#include "common/linealAlgebra/Vector3Dd.h"

VECTOR3Dd::VECTOR3Dd() {
    x = 0.0;
    y = 0.0;
    z = 0.0;
}

VECTOR3Dd::VECTOR3Dd(double inX, double inY, double inZ) {
    x = inX;
    y = inY;
    z = inZ;
}
/**
Normalize a vector, return old magnitude
*/
double
normalize(VECTOR3Dd *v, double epsilon)
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

    v->x /= len;
    v->y /= len;
    v->z /= len;

    return len;
}

/**
Cross product of two vectors
result = a X b
*/
void
floatCrossProduct(VECTOR3Dd *result, const VECTOR3Dd *a, const VECTOR3Dd *b)
{
    result->x = a->y * b->z - a->z * b->y;
    result->y = a->z * b->x - a->x * b->z;
    result->z = a->x * b->y - a->y * b->x;
}

/**
Returns squared distance between the two vectors
*/
double
distanceSquared(const VECTOR3Dd *v1, const VECTOR3Dd *v2) {
    VECTOR3Dd d;

    d.x = (*v2).x - (*v1).x;
    d.y = (*v2).y - (*v1).y;
    d.z = (*v2).z - (*v1).z;
    return d.x * d.x + d.y * d.y + d.z * d.z;
}
