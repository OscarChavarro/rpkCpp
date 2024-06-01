#ifndef __VECTOR_3D__
#define __VECTOR_3D__

#include <cmath>
#include <cstdio>

#include "java/lang/Math.h"
#include "common/linealAlgebra/Numeric.h"
#include "common/linealAlgebra/CoordinateAxis.h"

extern const int X_GREATER_MASK;
extern const int Y_GREATER_MASK;
extern const int Z_GREATER_MASK;
extern const int XYZ_EQUAL_MASK;

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
    CoordinateAxis dominantCoordinate() const;
    int compareByDimensions(const Vector3D *other, float epsilon) const;
    void print(FILE *fp) const;
    float dotProduct(Vector3D b) const;
    float norm2() const;
    float norm() const;
    float distance(const Vector3D &p2) const;
    float distance2(const Vector3D &p2) const;

    void set(float xParam, float yParam, float zParam);
    void copy(const Vector3D &v);
    void combine(float a, const Vector3D &v, float b, const Vector3D &w);
    void addition(const Vector3D &a, const Vector3D &b);
    void subtraction(const Vector3D& a, const Vector3D& b);
    void sumScaled(Vector3D a, double s, Vector3D b);
    void scaledCopy(float s, const Vector3D &v);
    void inverseScaledCopy(float s, const Vector3D &v, float epsilon);
    void normalize(float epsilon);
    void crossProduct(const Vector3D &a, const Vector3D &b);
    void combine3(const Vector3D &o, float a, const Vector3D &v, float b, const Vector3D &w);
    void tripleCrossProduct(const Vector3D &v1, const Vector3D &v2, const Vector3D &v3);
    void midPoint(const Vector3D &p1, const Vector3D &p2);
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
    return epsilon * (java::Math::abs(x) + java::Math::abs(y) + java::Math::abs(z));
}

/**
Two vectors are equal if their components are equal within the given tolerance
*/
inline bool
Vector3D::equals(const Vector3D &w, const float epsilon) const {
    return (
        Numeric::doubleEqual(x, w.x, epsilon) &&
        Numeric::doubleEqual(y, w.y, epsilon) &&
        Numeric::doubleEqual(z, w.z, epsilon)
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
Vector3D::norm() const {
    return java::Math::sqrt(norm2());
}

/**
Scale a vector: d = s.v (s is a real number)
*/
inline void
Vector3D::scaledCopy(const float s, const Vector3D &v) {
    x = s * v.x;
    y = s * v.y;
    z = s * v.z;
}

/**
Scales a vector with the inverse of the real number s if not zero: d = (1/s).v
*/
inline void
Vector3D::inverseScaledCopy(const float s, const Vector3D &v, const float epsilon) {
    float normalizedFactor = s < -epsilon || s > epsilon ? 1.0f / s : 1.0f;
    x = normalizedFactor * v.x;
    y = normalizedFactor * v.y;
    z = normalizedFactor * v.z;
}

/**
Normalizes a vector: scale it with the inverse of its norm
*/
inline void
Vector3D::normalize(const float epsilon) {
    float n = norm();
    inverseScaledCopy(n, *this, epsilon);
}

/**
In product of two vectors
*/
inline void
Vector3D::crossProduct(const Vector3D &a, const Vector3D &b) {
    x = a.y * b.z - a.z * b.y;
    y = a.z * b.x - a.x * b.z;
    z = a.x * b.y - a.y * b.x;
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
Vector3D::combine3(
    const Vector3D &o,
    const float a,
    const Vector3D &v,
    const float b,
    const Vector3D &w)
{
    x = o.x + a * v.x + b * w.x;
    y = o.y + a * v.y + b * w.y;
    z = o.z + a * v.z + b * w.z;
}

/**
Triple (cross) product: d = (v3 - v2) x (v1 - v2)
*/
inline void
Vector3D::tripleCrossProduct(
    const Vector3D &v1,
    const Vector3D &v2,
    const Vector3D &v3)
{
    Vector3D d1;
    Vector3D d2;
    d1.subtraction(v3, v2);
    d2.subtraction(v1, v2);
    crossProduct(d1, d2);
}

/**
Distance between two positions in 3D space: s = | p2 - p1 |
*/
inline float
Vector3D::distance(const Vector3D &p2) const {
    Vector3D d;
    d.subtraction(p2, *this);
    return d.norm();
}

/**
Squared distance between two positions in 3D space: s = |p2-p1|
*/
inline float
Vector3D::distance2(const Vector3D &p2) const {
    Vector3D d;
    d.subtraction(p2, *this);
    return d.norm2();
}

/**
Centre of two positions
*/
inline void
Vector3D::midPoint(const Vector3D &p1, const Vector3D &p2) {
    x = 0.5f * (p1.x + p2.x);
    y = 0.5f * (p1.y + p2.y);
    z = 0.5f * (p1.z + p2.z);
}

#endif
