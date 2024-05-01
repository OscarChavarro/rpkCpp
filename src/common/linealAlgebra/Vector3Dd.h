#ifndef __VECTOR3DD__
#define __VECTOR3DD__

#include "common/linealAlgebra/Float.h"

// Should be changed to Vector3Dd
typedef double VECTOR3Dd[3];

typedef double MATRIX4Dd[4][4];

#define MAT4IDENT { {1.0, 0.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 0.0}, \
                {0.0, 0.0, 1.0, 0.0}, {0.0, 0.0, 0.0, 1.0} }

extern double normalize(double *v);
extern void floatCrossProduct(VECTOR3Dd result, const VECTOR3Dd a, const VECTOR3Dd b);
extern void mgfMakeAxes(double *u, double *v, const double *w);
extern void multiplyP3(double *p3a, const double *p3b, double (*m4)[4]);
extern void multiplyV3(double *v3a, const double *v3b, double (*m4)[4]);
extern void multiplyMatrix4(double (*m4a)[4], double (*m4b)[4], double (*m4c)[4]);
extern void copyMat4(double (*m4a)[4], MATRIX4Dd m4b);
extern void setIdent4(double (*m4a)[4]);
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
