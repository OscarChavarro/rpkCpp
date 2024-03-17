#ifndef __Vector2Dd__
#define __Vector2Dd__

#include "common/linealAlgebra/Float.h"

class Vector2Dd {
public:
    double u;
    double v;

    Vector2Dd() {
        u = 0.0;
        v = 0.0;
    }

    Vector2Dd(double x, double y) {
        u = x;
        v = y;
    }

    inline bool operator==(const Vector2Dd &v) const;
    inline bool operator!=(const Vector2Dd &v) const;
    inline const Vector2Dd &operator+=(const Vector2Dd &v);
    inline const Vector2Dd &operator-=(const Vector2Dd &v);
    inline const Vector2Dd &operator*=(const Vector2Dd &v);
    inline const Vector2Dd &operator/=(const Vector2Dd &v);
    inline const Vector2Dd &operator*=(float s);
    inline const Vector2Dd &operator/=(float s);
    inline Vector2Dd operator+(const Vector2Dd &v) const;
    inline Vector2Dd operator-(const Vector2Dd &v) const;
    inline Vector2Dd operator-() const;
    inline Vector2Dd operator*(const Vector2Dd &v) const;
    inline Vector2Dd operator*(float s) const;
    inline Vector2Dd operator/(const Vector2Dd &v) const;
    inline Vector2Dd operator/(float s) const;
    inline double operator&(const Vector2Dd &r) const;
    inline double operator^(const Vector2Dd &r) const;
};

inline bool Vector2Dd::operator==(const Vector2Dd &V) const { return u == V.u && v == V.v; }

inline bool Vector2Dd::operator!=(const Vector2Dd &V) const { return u != V.u || v != V.v; }

inline const Vector2Dd &Vector2Dd::operator+=(const Vector2Dd &V) {
    u += V.u;
    v += V.v;
    return *this;
}

inline const Vector2Dd &Vector2Dd::operator-=(const Vector2Dd &V) {
    u -= V.u;
    v -= V.v;
    return *this;
}

inline const Vector2Dd &Vector2Dd::operator*=(const Vector2Dd &V) {
    u *= V.u;
    v *= V.v;
    return *this;
}

inline const Vector2Dd &Vector2Dd::operator/=(const Vector2Dd &V) {
    u /= V.u;
    v /= V.v;
    return *this;
}

inline const Vector2Dd &Vector2Dd::operator*=(float s) {
    u *= s;
    v *= s;
    return *this;
}

inline const Vector2Dd &Vector2Dd::operator/=(float s) {
    u /= s;
    v /= s;
    return *this;
}

inline Vector2Dd Vector2Dd::operator-() const {
    return {-u, -v};
}

inline Vector2Dd Vector2Dd::operator+(const Vector2Dd &V) const {
    return {u + V.u, v + V.v};
}

inline Vector2Dd Vector2Dd::operator-(const Vector2Dd &V) const {
    return {u - V.u, v - V.v};
}

inline Vector2Dd Vector2Dd::operator*(const Vector2Dd &V) const {
    return {u * V.u, v * V.v};
}

inline Vector2Dd Vector2Dd::operator/(const Vector2Dd &V) const {
    return {u / V.u, v / V.v};
}

inline Vector2Dd Vector2Dd::operator*(float s) const {
    return {u * s, v * s};
}

inline Vector2Dd Vector2Dd::operator/(float s) const {
    return {u / s, v / s};
}

inline double Vector2Dd::operator&(const Vector2Dd &r) const {
    return u * r.u + v * r.v;
}

inline double Vector2Dd::operator^(const Vector2Dd &r) const {
    return u * r.v - v * r.u;
}

inline
void vector2DSet(Vector2Dd &v, double a, double b) {
    v.u = a;
    v.v = b;
}

inline void
vector2DSubtract(Vector2Dd &p, Vector2Dd &q, Vector2Dd &r) {
    r.u = p.u - q.u;
    r.v = p.v - q.v;
}

inline void
vector2DAdd(Vector2Dd &p, Vector2Dd &q, Vector2Dd &r) {
    r.u = p.u + q.u;
    r.v = p.v + q.v;
}

inline void
vector2DNegate(Vector2Dd &p) {
    p.u = -p.u;
    p.v = -p.v;
}

inline REAL
vector2DDeterminant(Vector2Dd &A, Vector2Dd &B) {
    return ((REAL)A.u * (REAL)B.v - (REAL)A.v * (REAL)B.u);
}

#endif
