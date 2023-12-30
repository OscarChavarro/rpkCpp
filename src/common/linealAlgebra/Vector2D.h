#ifndef __Vector2D__
#define __Vector2D__

class Vector2D {
public:
    float u, v;

    Vector2D() {
        u = 0.0;
        v = 0.0;
    }

    Vector2D(float x, float y) {
        u = x;
        v = y;
    }

    inline bool operator==(const Vector2D &v) const;
    inline bool operator!=(const Vector2D &v) const;
    inline const Vector2D &operator+=(const Vector2D &v);
    inline const Vector2D &operator-=(const Vector2D &v);
    inline const Vector2D &operator*=(const Vector2D &v);
    inline const Vector2D &operator/=(const Vector2D &V);
    inline const Vector2D &operator*=(float s);
    inline const Vector2D &operator/=(float s);
    inline Vector2D operator+(const Vector2D &v) const;
    inline Vector2D operator-(const Vector2D &v) const;
    inline Vector2D operator-() const;
    inline Vector2D operator*(const Vector2D &v) const;
    inline Vector2D operator*(float s) const;
    inline Vector2D operator/(const Vector2D &v) const;
    inline Vector2D operator/(float s) const;
    inline float operator&(const Vector2D &r) const;
    inline float operator^(const Vector2D &r) const;
};

inline bool Vector2D::operator==(const Vector2D &V) const {
    return u == V.u && v == V.v;
}

inline bool Vector2D::operator!=(const Vector2D &V) const {
    return u != V.u || v != V.v;
}

inline const Vector2D &Vector2D::operator+=(const Vector2D &V) {
    u += V.u;
    v += V.v;
    return *this;
}

inline const Vector2D &Vector2D::operator-=(const Vector2D &V) {
    u -= V.u;
    v -= V.v;
    return *this;
}

inline const Vector2D &Vector2D::operator*=(const Vector2D &V) {
    u *= V.u;
    v *= V.v;
    return *this;
}

inline const Vector2D &Vector2D::operator/=(const Vector2D &V) {
    u /= V.u;
    v /= V.v;
    return *this;
}

inline const Vector2D &Vector2D::operator*=(float s) {
    u *= s;
    v *= s;
    return *this;
}

inline const Vector2D &Vector2D::operator/=(float s) {
    u /= s;
    v /= s;
    return *this;
}

inline Vector2D Vector2D::operator-() const {
    return {-u, -v};
}

inline Vector2D Vector2D::operator+(const Vector2D &V) const {
    return {u + V.u, v + V.v};
}

inline Vector2D Vector2D::operator-(const Vector2D &V) const {
    return {u - V.u, v - V.v};
}

inline Vector2D Vector2D::operator*(const Vector2D &V) const {
    return {u * V.u, v * V.v};
}

inline Vector2D Vector2D::operator/(const Vector2D &V) const {
    return {u / V.u, v / V.v};
}

inline Vector2D Vector2D::operator*(float s) const {
    return {u * s, v * s};
}

inline Vector2D Vector2D::operator/(float s) const {
    return {u / s, v / s};
}

inline float Vector2D::operator&(const Vector2D &r) const {
    return u * r.u + v * r.v;
}

inline float Vector2D::operator^(const Vector2D &r) const {
    return u * r.v - v * r.u;
}

#endif
