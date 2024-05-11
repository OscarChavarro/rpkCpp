#ifndef __VECTOR_3D__
#define __VECTOR_3D__

#include <cstdio>

#include "common/linealAlgebra/Float.h"

class Vector3D {
  public:
    float x;
    float y;
    float z;

    Vector3D();
    Vector3D(float a, float b, float c) : x(a), y(b), z(c) {}

    Vector3D transform(const Vector3D &X, const Vector3D &Y, const Vector3D &Z) const;
    float tolerance(float epsilon) const;
    bool equals(const Vector3D &w, float epsilon) const;
    int dominantCoordinate() const;
    int compareByDimensions(const Vector3D *v2, float epsilon) const;
    void print(FILE *fp) const;
    float dotProduct(Vector3D b) const;
    float norm2() const;

    void set(float xParam, float yParam, float zParam);
    void copy(const Vector3D &v);
    void combine(float a, const Vector3D &v, float b, const Vector3D &w);
    void addition(const Vector3D &a, const Vector3D &b);
    void subtraction(const Vector3D& a, const Vector3D& b);
    void sumScaled(Vector3D a, double s, Vector3D b);
};

inline
Vector3D::Vector3D() {
    x = 0.0;
    y = 0.0;
    z = 0.0;
}

/**
Fills in x, y, and z component of a vector
*/
inline void
Vector3D::set(const float xParam, const float yParam, const float zParam) {
    x = xParam;
    y = yParam;
    z = zParam;
}

/**
Copies the vector v to d: d = v. They may be different vector types
*/
inline void
Vector3D::copy(const Vector3D &v) {
    x = v.x;
    y = v.y;
    z = v.z;
}

/**
Tolerance value for e.g. a vertex position
*/
inline float
Vector3D::tolerance(float epsilon) const {
    return epsilon * (std::fabs(x) + std::fabs(y) + std::fabs(z));
}

/**
Two vectors are equal if their components are equal within the given tolerance
*/
inline bool
Vector3D::equals(const Vector3D &w, const float epsilon) const {
    return (
        doubleEqual(x, w.x, epsilon) &&
        doubleEqual(y, w.y, epsilon) &&
        doubleEqual(z, w.z, epsilon)
    );
}

/**
Vector difference
*/
inline void
Vector3D::subtraction(const Vector3D& a, const Vector3D& b) {
    x = a.x - b.x;
    y = a.y - b.y;
    z = a.z - b.z;
}

/**
Vector sum: d = a + b
*/
inline void
Vector3D::addition(const Vector3D &a, const Vector3D &b) {
    x = a.x + b.x;
    y = a.y + b.y;
    z = a.z + b.z;
}

/**
Scaled vector sum: d = a + s.b
*/
inline void
Vector3D::sumScaled(const Vector3D a, const double s, const Vector3D b) {
    x = a.x + (float)s * b.x;
    y = a.y + (float)s * b.y;
    z = a.z + (float)s * b.z;
}

/**
Scalar vector product: a.b
*/
inline float
Vector3D::dotProduct(const Vector3D b) const {
    return x * b.x + y * b.y + z * b. z;
}

/**
Compute (T * vector) with
T = transpose[ X Y Z ] so that e.g. T.X = (1 0 0) if
X, Y, Z form a coordinate system
*/
inline Vector3D
Vector3D::transform(
    const Vector3D &X,
    const Vector3D &Y,
    const Vector3D &Z) const {
    return {
        X.dotProduct(*this),
        Y.dotProduct(*this),
        Z.dotProduct(*this),
    };
}

/**
Square of vector norm: scalar product with itself
*/
inline float
Vector3D::norm2() const {
    return x * x + y * y + z * z;
}

/**
Norm of a vector: square root of the square norm
*/
inline float
vectorNorm(const Vector3D &v) {
    return std::sqrt(v.norm2());
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

/**
Normalizes a vector: scale it with the inverse of its norm
*/
inline void
vectorNormalize(Vector3D &v) {
    float n = vectorNorm(v);
    vectorScaleInverse(n, v, v);
}

/**
In product of two vectors
*/
inline void
vectorCrossProduct(const Vector3D &a, const Vector3D &b, Vector3D &d) {
    Vector3D tmp;
    tmp.x = a.y * b.z - a.z * b.y;
    tmp.y = a.z * b.x - a.x * b.z;
    tmp.z = a.x * b.y - a.y * b.x;
    d.copy(tmp);
}

/**
Linear combination of two vectors: d = a.v + b.w
*/
inline void
Vector3D::combine(
    const float a,
    const Vector3D &v,
    const float b,
    const Vector3D &w)
{
    x = a * v.x + b * w.x;
    y = a * v.y + b * w.y;
    z = a * v.z + b * w.z;
}

/**
Affine linear combination of two vectors: d = o + a.v + b.w
*/
inline void
vectorComb3(
    const Vector3D &o,
    const float a,
    const Vector3D &v,
    const float b,
    const Vector3D &w,
    Vector3D &d)
{
    d.x = o.x + a * v.x + b * w.x;
    d.y = o.y + a * v.y + b * w.y;
    d.z = o.z + a * v.z + b * w.z;
}

/**
Triple (cross) product: d = (v3 - v2) x (v1 - v2)
*/
inline void
vectorTripleCrossProduct(
    const Vector3D &v1,
    const Vector3D &v2,
    const Vector3D &v3,
    Vector3D &d)
{
    Vector3D d1;
    Vector3D d2;
    d1.subtraction(v3, v2);
    d2.subtraction(v1, v2);
    vectorCrossProduct(d1, d2, d);
}

/**
Distance between two positions in 3D space: s = |p2-p1|
*/
inline float
vectorDist(const Vector3D &p1, const Vector3D &p2) {
    Vector3D d;
    d.subtraction(p2, p1);
    return vectorNorm(d);
}

/**
Squared distance between two positions in 3D space: s = |p2-p1|
*/
inline float
vectorDist2(const Vector3D &p1, const Vector3D &p2) {
    Vector3D d;
    d.subtraction(p2, p1);
    return d.norm2();
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
vectorPointInTriangle(
    const Vector3D &v0,
    const Vector3D &v1,
    const Vector3D &v2,
    const float u,
    const float v,
    Vector3D &p)
{
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
