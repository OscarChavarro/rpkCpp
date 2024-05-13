#ifndef __VECTOR3DD__
#define __VECTOR3DD__

#include "common/linealAlgebra/Float.h"

// Should be changed to Vector3Dd
typedef double VECTOR3Dd[3];

extern double normalize(double *v);
extern void floatCrossProduct(VECTOR3Dd result, const VECTOR3Dd a, const VECTOR3Dd b);
extern void mgfMakeAxes(double *u, double *v, const double *w);
extern double distanceSquared(VECTOR3Dd *v1, VECTOR3Dd *v2);

inline void
mgfVertexCopy(double *result, const double *source) {
    result[0] = source[0];
    result[1] = source[1];
    result[2] = source[2];
}

extern inline double
dotProduct(const double *a, const double *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

extern inline bool
is0Vector(const double *v) {
    return dotProduct(v, v) <= EPSILON * EPSILON;
}

extern inline void
round0(double &x) {
    if ( x <= EPSILON && x >= -EPSILON) {
        x = 0;
    }
}

#endif
