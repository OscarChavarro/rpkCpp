#ifndef __Vector3D__
#define __Vector3D__

#include <cstdio>
#include "common/linealAlgebra/Float.h"

class Vector3D {
  public:
    float x;
    float y;
    float z;

    Vector3D() {
        x = 0.0;
        y = 0.0;
        z = 0.0;
    }

    Vector3D(float a, float b, float c) : x(a), y(b), z(c) {}
    inline bool operator==(const Vector3D &v) const;
    inline bool operator!=(const Vector3D &v) const;

    // Precondition: 0 <= i < 3
    inline float operator[](int i);

    inline const Vector3D &operator+=(const Vector3D &v);
    inline const Vector3D &operator-=(const Vector3D &v);
    inline const Vector3D &operator*=(const Vector3D &v);
    inline const Vector3D &operator/=(const Vector3D &v);
    inline const Vector3D &operator*=(float s);
    inline const Vector3D &operator/=(float s);
    inline Vector3D operator+(const Vector3D &v) const;
    inline Vector3D operator-(const Vector3D &v) const;
    inline Vector3D operator-() const;
    inline Vector3D operator*(const Vector3D &v) const;
    inline Vector3D operator*(float s) const;
    inline Vector3D operator/(const Vector3D &v) const;
    inline Vector3D operator/(float s) const;

    /**
    Compute (T * vector) with
    T = transpose[ X Y Z ] so that e.g. T.X = (1 0 0) if
    X, Y, Z form a coordinate system
    */
    inline Vector3D transform(const Vector3D &X,
                              const Vector3D &Y,
                              const Vector3D &Z) const;

    /**
    Fills in x, y, and z component of a vector
    */
    inline void set(const float xParam,
                    const float yParam,
                    const float zParam) {
        x = xParam;
        y = yParam;
        z = zParam;
    }
};

inline bool
Vector3D::operator==(const Vector3D &v) const {
    return x == v.x && y == v.y && z == v.z;
}

inline bool
Vector3D::operator!=(const Vector3D &v) const {
    return x != v.x || y != v.y || z != v.z;
}

inline float
Vector3D::operator[](int i) {
    return ((float *) this)[i];
}

inline const Vector3D &
Vector3D::operator+=(const Vector3D &v) {
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}

inline const Vector3D &
Vector3D::operator-=(const Vector3D &v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

inline const Vector3D &
Vector3D::operator*=(const Vector3D &v) {
    x *= v.x;
    y *= v.y;
    z *= v.z;
    return *this;
}

inline const Vector3D &
Vector3D::operator/=(const Vector3D &v) {
    x /= v.x;
    y /= v.y;
    z /= v.z;
    return *this;
}

inline const Vector3D &
Vector3D::operator*=(float s) {
    x *= s;
    y *= s;
    z *= s;
    return *this;
}

inline const Vector3D &
Vector3D::operator/=(float s) {
    x /= s;
    y /= s;
    z /= s;
    return *this;
}

inline Vector3D Vector3D::operator-() const {
    return {-x, -y, -z};
}

inline Vector3D Vector3D::operator+(const Vector3D &v) const {
    return {x + v.x, y + v.y, z + v.z};
}

inline Vector3D Vector3D::operator-(const Vector3D &v) const {
    return {x - v.x, y - v.y, z - v.z};
}

inline Vector3D Vector3D::operator*(const Vector3D &v) const {
    return {x * v.x, y * v.y, z * v.z};
}

inline Vector3D Vector3D::operator/(const Vector3D &v) const {
    return {x / v.x, y / v.y, z / v.z};
}

inline Vector3D Vector3D::operator*(float s) const {
    return {x * s, y * s, z * s};
}

inline Vector3D Vector3D::operator/(float s) const {
    return {x / s, y / s, z / s};
}

// Scalar times vector
inline Vector3D operator*(float s, const Vector3D &v) {
    return {v.x * s, v.y * s, v.z * s};
}

// Dot product (Warning: '&' has lower precedence than + -)
inline float operator&(const Vector3D &l, const Vector3D &r) {
    return l.x * r.x + l.y * r.y + l.z * r.z;
}

// Cross product (Warning: '^' has lower precedence than + -)
inline Vector3D operator^(const Vector3D &l, const Vector3D &r) {
    return {l.y * r.z - l.z * r.y,
            l.z * r.x - l.x * r.z,
            l.x * r.y - l.y * r.x};
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
    return {X & *this, Y & *this, Z & *this};
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

/**
Tolerance value for e.g. a vertex position
*/
inline float
vectorTolerance(const Vector3D v) {
    return EPSILON * (std::fabs(v.x) + std::fabs(v.y) + std::fabs(v.z));
}

/**
Two vectors are equal if their components are equal within the given tolerance
*/
inline bool
vectorEqual(const Vector3D &v, const Vector3D &w, const float eps) {
    return (
            doubleEqual(v.x, w.x, eps) &&
            doubleEqual(v.y, w.y, eps) &&
            doubleEqual(v.z, w.z, eps)
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

/**
Scalar vector product: a.b
*/
inline float
vectorDotProduct(const Vector3D a, const Vector3D b) {
    return a.x * b.x + a.y * b.y + a.z * b. z;
}

/**
Square of vector norm: scalar product with itself
*/
inline float
vectorNorm2(const Vector3D &v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

/**
Norm of a vector: square root of the square norm
*/
inline float
vectorNorm(const Vector3D &v) {
    return (float)std::sqrt((double) vectorNorm2(v));
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
    vectorCopy(tmp, d);
}

/**
Linear combination of two vectors: d = a.v + b.w
*/
inline void
vectorComb2(
    const float a,
    const Vector3D &v,
    const float b,
    const Vector3D &w,
    Vector3D &d)
{
    d.x = a * v.x + b * w.x;
    d.y = a * v.y + b * w.y;
    d.z = a * v.z + b * w.z;
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
    Vector3D d;
    vectorSubtract(p2, p1, d);
    return vectorNorm2(d);
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

extern int vectorCompareByDimensions(Vector3D *v1, Vector3D *v2, float epsilon);
extern void vector3DDestroy(Vector3D *vector);
extern int vector3DDominantCoord(const Vector3D *v);
extern void vector3DPrint(FILE *fp, Vector3D &v);

#endif
