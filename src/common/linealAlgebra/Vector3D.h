#ifndef __Vector3D__
#define __Vector3D__

#include <cstdio>
#include <cstdlib>

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

    /* Compute (T * vector) with  */
    /* T = transpose[ X Y Z ] so that e.g. T.X = (1 0 0) if */
    /* X,Y,Z form a coordinate system */
    inline Vector3D transform(const Vector3D &X,
                              const Vector3D &Y,
                              const Vector3D &Z) const;
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

// Compute (T * vector) with
// T = transpose[ X Y Z ] so that e.g. T.X = (1 0 0) if
// X,Y,Z form a coordinate system
inline Vector3D
Vector3D::transform(
        const Vector3D &X,
        const Vector3D &Y,
        const Vector3D &Z) const {
    return {X & *this, Y & *this, Z & *this};
}

inline Vector3D *
VectorCreate(float x, float y, float z)
{
    Vector3D *vector;

    vector = (Vector3D *)malloc(sizeof(Vector3D));

    vector->x = x;
    vector->y = y;
    vector->z = z;

    return vector;
}

extern int vectorCompareByDimensions(Vector3D *v1, Vector3D *v2, float epsilon);
extern void vector3DDestroy(Vector3D *vector);
extern int vector3DDominantCoord(Vector3D *v);
extern void vector3DPrint(FILE *fp, Vector3D &v);

#endif
