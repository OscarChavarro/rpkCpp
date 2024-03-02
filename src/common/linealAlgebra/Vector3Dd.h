#ifndef __Vector3Dd__
#define __Vector3Dd__

/**
Double floating point vectors: The extra precision is sometimes needed eg
for sampling a spherical triangle or computing an analytical point to
patch factor, also for accurately computing normals and areas
*/
class Vector3Dd {
  public:
    double x;
    double y;
    double z;

    Vector3Dd() {
        x = 0.0;
        y = 0.0;
        z = 0.0;
    }
};

/**
Fills in x, y, and z component of a vector
*/
inline void
vectorSet(Vector3Dd &v, const double a, const double b, const double c) {
    v.x = a;
    v.y = b;
    v.z = c;
}

/**
Vector difference
*/
inline void
vectorSubtract(const Vector3Dd& a, const Vector3Dd& b, Vector3Dd& d) {
    d.x = a.x - b.x;
    d.y = a.y - b.y;
    d.z = a.z - b.z;
}

/**
Scaled vector sum: d = a + s.b
*/
inline void
vectorSumScaled(const Vector3Dd a, const double s, const Vector3Dd b, Vector3Dd &d) {
    d.x = a.x + s * b.x;
    d.y = a.y + s * b.y;
    d.z = a.z + s * b.z;
}

/**
Scalar vector product: a.b
*/
inline double
vectorDotProduct(const Vector3Dd a, const Vector3Dd b) {
    return a.x * b.x + a.y * b.y + a.z * b. z;
}

/**
Square of vector norm: scalar product with itself
*/
inline float
vectorNorm2(const Vector3Dd &v) {
    return (float)(v.x * v.x + v.y * v.y + v.z * v.z);
}

/**
Norm of a vector: square root of the square norm
*/
inline double
vectorNorm(const Vector3Dd &v) {
    return (double)std::sqrt((double) vectorNorm2(v));
}

/**
Scale a vector: d = s.v (s is a real number)
*/
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
vectorNormalize(Vector3Dd &v) {
    double n = vectorNorm(v);
    vectorScaleInverse(n, v, v);
}

#endif
