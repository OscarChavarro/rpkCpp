#ifndef __VECTOR_4_DF__
#define __VECTOR_4_DF__


#include <cmath>
class Vector3Df;
class Vector4Dd;

class Vector4Df {
    float x_;
    float y_;
    float z_;
    float w_;

public:
    Vector4Df(float x, float y, float z, float w) : x_(x), y_(y), z_(z), w_(w) {}
    Vector4Df(const Vector4Df& other) : x_(other.x_), y_(other.y_), z_(other.z_), w_(other.w_) {}
    explicit Vector4Df(const Vector4Dd& other);
    explicit Vector4Df(const Vector3Df& other);

    Vector4Df multiply(float a) const;
    Vector4Df dividedByW() const;
    float length() const;
    Vector4Df add(const Vector4Df& b) const;

    Vector4Df withX(float nx) const { return Vector4Df(nx, y_, z_, w_); }
    Vector4Df withY(float ny) const { return Vector4Df(x_, ny, z_, w_); }
    Vector4Df withZ(float nz) const { return Vector4Df(x_, y_, nz, w_); }
    Vector4Df withW(float nw) const { return Vector4Df(x_, y_, z_, nw); }
    float x() const { return x_; }
    float y() const { return y_; }
    float z() const { return z_; }
    float w() const { return w_; }

    bool epsilonEquals(const Vector4Df& other) const { return epsilonEquals(other, 1e-6f); }
    bool epsilonEquals(const Vector4Df& other, float epsilon) const;

    bool operator==(const Vector4Df& other) const { return x_ == other.x_ && y_ == other.y_ && z_ == other.z_ && w_ == other.w_; }
};


#endif
