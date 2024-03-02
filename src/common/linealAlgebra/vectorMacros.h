#ifndef __VECTOR_FUNCTION__
#define __VECTOR_FUNCTION__

#include "common/linealAlgebra/Vector2D.h"
#include "common/linealAlgebra/Vector2Dd.h"
#include "common/linealAlgebra/Vector3D.h"
#include "common/linealAlgebra/Vector3Dd.h"

#define X_NORMAL 0
#define Y_NORMAL 1
#define Z_NORMAL 2
#define X_GREATER 0x01
#define Y_GREATER 0x02
#define Z_GREATER 0x04
#define XYZ_EQUAL 0x08

/**
Fills in x, y, and z component of a vector
*/
inline void
vectorSet(Vector3D &v, const float a, const float b, const float c) {
    v.x = a;
    v.y = b;
    v.z = c;
}

inline void
vectorSet(Vector3Dd &v, const double a, const double b, const double c) {
    v.x = a;
    v.y = b;
    v.z = c;
}

/**
Copies the vector v to d: d = v. They may be different vector types
*/
inline void
vectorCopy(const Vector3D &v, Vector3D &d) {
    d.x = v.x;
    d.y = v.y;
    d.z = v.z;
}

inline void
vectorCopy(const Vector3Dd &v, Vector3D &d) {
    d.x = (float)v.x;
    d.y = (float)v.y;
    d.z = (float)v.z;
}

/**
Tolerance value for e.g. a vertex position
*/
inline float
vectorTolerance(const Vector3D v) {
    return EPSILON * (fabs(v.x) + fabs(v.y) + fabs(v.z));
}

/**
Two vectors are equal if their components are equal within the given tolerance
*/
inline bool
vectorEqual(const Vector3D &v, const Vector3D &w, const float eps) {
    return (
        floatEqual(v.x, w.x, eps) &&
        floatEqual(v.y, w.y, eps) &&
        floatEqual(v.z, w.z, eps)
    );
}

/**
Vector difference
*/
inline void
vectorSubtract(const Vector3D& a, const Vector3D& b, Vector3D& d) {
    d.x = a.x - b.x;
    d.y = a.y - b.y;
    d.z = a.z - b.z;
}

inline void
vectorSubtract(const Vector3Dd& a, const Vector3Dd& b, Vector3Dd& d) {
    d.x = a.x - b.x;
    d.y = a.y - b.y;
    d.z = a.z - b.z;
}

inline void
vectorSubtract(const Vector3Dd& a, const Vector3Dd& b, Vector3D& d) {
    d.x = (float)(a.x - b.x);
    d.y = (float)(a.y - b.y);
    d.z = (float)(a.z - b.z);
}

inline void
vectorSubtract(const Vector3D& a, const Vector3D& b, Vector3Dd& d) {
    d.x = a.x - b.x;
    d.y = a.y - b.y;
    d.z = a.z - b.z;
}

/**
Vector sum: d = a + b
*/
inline void
vectorAdd(const Vector3D &a, const Vector3D &b, Vector3D &d) {
    d.x = a.x + b.x;
    d.y = a.y + b.y;
    d.z = a.z + b.z;
}

/**
Scaled vector sum: d = a + s.b
*/
inline void
vectorSumScaled(const Vector3D a, const double s, const Vector3D b, Vector3D &d) {
    d.x = a.x + (float)s * b.x;
    d.y = a.y + (float)s * b.y;
    d.z = a.z + (float)s * b.z;
}

inline void
vectorSumScaled(const Vector3D a, const double s, const Vector3D b, Vector3Dd &d) {
    d.x = a.x + s * b.x;
    d.y = a.y + s * b.y;
    d.z = a.z + s * b.z;
}

inline void
vectorSumScaled(const Vector3Dd a, const double s, const Vector3Dd b, Vector3Dd &d) {
    d.x = a.x + s * b.x;
    d.y = a.y + s * b.y;
    d.z = a.z + s * b.z;
}

/**
Scalar vector product: a.b
*/
inline float
vectorDotProduct(const Vector3D a, const Vector3D b) {
    return a.x * b.x + a.y * b.y + a.z * b. z;
}

inline double
vectorDotProduct(const Vector3Dd a, const Vector3Dd b) {
    return a.x * b.x + a.y * b.y + a.z * b. z;
}

inline double
vectorDotProduct(const Vector3Dd a, const Vector3D b) {
    return a.x * b.x + a.y * b.y + a.z * b. z;
}

/**
Square of vector norm: scalar product with itself
*/
inline float
vectorNorm2(const Vector3D &v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline float
vectorNorm2(const Vector3Dd &v) {
    return (float)(v.x * v.x + v.y * v.y + v.z * v.z);
}

/**
Norm of a vector: square root of the square norm
*/
inline float
vectorNorm(const Vector3D &v) {
    return (float)std::sqrt((double) vectorNorm2(v));
}

inline double
vectorNorm(const Vector3Dd &v) {
    return (double)std::sqrt((double) vectorNorm2(v));
}

/**
Scale a vector: d = s.v (s is a real number)
*/
inline void
vectorScale(const float s, const Vector3D &v, Vector3D &d) {
    d.x = s * v.x;
    d.y = s * v.y;
    d.z = s * v.z;
}

inline void
vectorScale(const double s, const Vector3Dd &v, Vector3Dd &d) {
    d.x = s * v.x;
    d.y = s * v.y;
    d.z = s * v.z;
}

/**
Scales a vector with the inverse of the real number s if not zero: d = (1/s).v
*/
inline void
vectorScaleInverse(const float s, const Vector3D &v, Vector3D &d) {
    float normalizedFactor = s < -EPSILON || s > EPSILON ? 1.0f/s : 1.0f;
    d.x = normalizedFactor * v.x;
    d.y = normalizedFactor * v.y;
    d.z = normalizedFactor * v.z;
}

inline void
vectorScaleInverse(const double s, const Vector3Dd &v, Vector3Dd &d) {
    double normalizedFactor = s < -EPSILON || s > EPSILON ? 1.0/s : 1.0;
    d.x = normalizedFactor * v.x;
    d.y = normalizedFactor * v.y;
    d.z = normalizedFactor * v.z;
}

/**
Normalizes a vector: scale it with the inverse of its norm
*/
inline void
vectorNormalize(Vector3D &v) {
    float n = vectorNorm(v);
    vectorScaleInverse(n, v, v);
}

inline void
vectorNormalize(Vector3Dd &v) {
    double n = vectorNorm(v);
    vectorScaleInverse(n, v, v);
}

/**
In product of two vectors
*/
inline void
vectorCrossProduct(const Vector3D &a, const Vector3D &b, Vector3D &d) {
    Vector3Dd tmp;
    tmp.x = a.y * b.z - a.z * b.y;
    tmp.y = a.z * b.x - a.x * b.z;
    tmp.z = a.x * b.y - a.y * b.x;
    vectorCopy(tmp, d);
}

/**
Linear combination of two vectors: d = a.v + b.w
*/
inline void
vectorComb2(const float a, const Vector3D &v, const float b, const Vector3D &w, Vector3D &d) {
    d.x = a * v.x + b * w.x;
    d.y = a * v.y + b * w.y;
    d.z = a * v.z + b * w.z;
}

/**
Affine linear combination of two vectors: d = o + a.v + b.w
*/
inline void
vectorComb3(const Vector3D &o, const float a, const Vector3D &v, const float b, const Vector3D &w, Vector3D &d) {
    d.x = o.x + a * v.x + b * w.x;
    d.y = o.y + a * v.y + b * w.y;
    d.z = o.z + a * v.z + b * w.z;
}

/**
Triple (cross) product: d = (v3 - v2) x (v1 - v2)
*/
inline void
vectorTripleCrossProduct(const Vector3D &v1, const Vector3D &v2, const Vector3D &v3, Vector3D &d) {
    Vector3D d1;
    Vector3D d2;
    vectorSubtract(v3, v2, d1);
    vectorSubtract(v1, v2, d2);
    vectorCrossProduct(d1, d2, d);
}

/**
Distance between two positions in 3D space: s = |p2-p1|
*/
inline float
vectorDist(const Vector3D &p1, const Vector3D &p2) {
    Vector3D d;
    vectorSubtract(p2, p1, d);
    return vectorNorm(d);
}

/**
Squared distance between two positions in 3D space: s = |p2-p1|
*/
inline float
vectorDist2(const Vector3D &p1, const Vector3D &p2) {
    Vector3Dd d;
    vectorSubtract(p2, p1, d);
    return vectorNorm2(d);
}

/**
Given a vector p in 3D space and an index i, which is X_NORMAL, Y_NORMAL
or Z_NORMAL, projects the vector on the YZ, XZ or XY plane respectively
*/
inline void
vectorProject(Vector2Dd &r, Vector3D &p, int i) {
    switch ( i ) {
        case X_NORMAL:
            r.u = p.y;
            r.v = p.z;
            break;
        case Y_NORMAL:
            r.u = p.x;
            r.v = p.z;
            break;
        case Z_NORMAL:
            r.u = p.x;
            r.v = p.y;
            break;
        default:
            break;
    }
}

inline void
vectorProject(Vector2D &r, Vector3D &p, int i) {
    switch(i) {
        case X_NORMAL:
            r.u = p.y;
            r.v = p.z;
            break;
        case Y_NORMAL:
            r.u = p.x;
            r.v = p.z;
            break;
        case Z_NORMAL:
            r.u = p.x;
            r.v = p.y;
            break;
        default:
            break;
    }
}

/**
Centre of two positions
*/
inline void
vectorMidPoint(const Vector3D &p1, const Vector3D &p2, Vector3D &m) {
    m.x = 0.5f * (p1.x + p2.x);
    m.y = 0.5f * (p1.y + p2.y);
    m.z = 0.5f * (p1.z + p2.z);
}

/**
Point IN Triangle: barycentric parametrisation
*/
inline void
vectorPointInTriangle(const Vector3D &v0, const Vector3D &v1, const Vector3D &v2, const float u, const float v, Vector3D &p) {
    p.x = v0.x + u * (v1.x - v0.x) + v * (v2.x - v0.x);
    p.y = v0.y + u * (v1.y - v0.y) + v * (v2.y - v0.y);
    p.z = v0.z + u * (v1.z - v0.z) + v * (v2.z - v0.z);
}

/**
Point IN Quadrilateral: bi-linear parametrisation
*/
inline void
vectorPointInQuadrilateral(
    const Vector3D &v0,
    const Vector3D &v1,
    const Vector3D &v2,
    const Vector3D &v3,
    const float u,
    const float v,
    Vector3D &p)
{
    float c = u * v;
    float b = u - c;
    float d = v - c;
    p.x = v0.x + b * (v1.x - v0.x) + c * (v2.x - v0.x) + d * (v3.x - v0.x);
    p.y = v0.y + b * (v1.y - v0.y) + c * (v2.y - v0.y) + d * (v3.y - v0.y);
    p.z = v0.z + b * (v1.z - v0.z) + c * (v2.z - v0.z) + d * (v3.z - v0.z);
}

#endif
