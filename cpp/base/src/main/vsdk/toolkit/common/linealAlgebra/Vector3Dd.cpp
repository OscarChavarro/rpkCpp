/**
Routines for 3-d vectors
*/

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

Vector3Dd::Vector3Dd() {
    x = 0.0;
    y = 0.0;
    z = 0.0;
}

Vector3Dd::Vector3Dd(double inX, double inY, double inZ) {
    x = inX;
    y = inY;
    z = inZ;
}

Vector3Dd::~Vector3Dd() {
}

/**
Normalize a vector, return old magnitude
*/
double
Vector3Dd::normalizeAndGivePreviousNorm(double epsilon)
{
    double len = this->dotProduct(this); // Note: this starts being length ^ 2

    if ( len <= 0.0 ) {
        return 0.0;
    }

    if ( len <= 1.0 + epsilon && len >= 1.0 - epsilon) {
        // First order approximation
        len = 0.5 + 0.5 * len;
    } else {
        len = java::Math::sqrt(len);
    }

    x /= len;
    y /= len;
    z /= len;

    return len;
}

/**
Cross product of two vectors
result = a X b
*/
void
Vector3Dd::crossProduct(const Vector3Dd *a, const Vector3Dd *b)
{
    x = a->y * b->z - a->z * b->y;
    y = a->z * b->x - a->x * b->z;
    z = a->x * b->y - a->y * b->x;
}

/**
Returns squared distance between the two vectors
*/
double
Vector3Dd::distanceSquared(const Vector3Dd *v2) const {
    Vector3Dd d;

    d.x = v2->x - x;
    d.y = v2->y - y;
    d.z = v2->z - z;
    return d.x * d.x + d.y * d.y + d.z * d.z;
}
