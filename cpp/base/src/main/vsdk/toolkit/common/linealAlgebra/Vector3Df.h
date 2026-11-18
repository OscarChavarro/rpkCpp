#ifndef __VECTOR_3_DF__
#define __VECTOR_3_DF__


#include <cmath>
class Vector3Dd;

class Vector3Df {
    float x_;
    float y_;
    float z_;

public:
    Vector3Df() : x_(0.0f), y_(0.0f), z_(0.0f) {}
    Vector3Df(float x, float y, float z) : x_(x), y_(y), z_(z) {}
    Vector3Df(const Vector3Df& other) : x_(other.x_), y_(other.y_), z_(other.z_) {}
    explicit Vector3Df(const Vector3Dd& other);

    Vector3Df multiply(float a) const;
    Vector3Df crossProduct(const Vector3Df& other) const;
    float dotProduct(const Vector3Df& other) const;
    Vector3Df normalized() const;
    float length() const;
    Vector3Df add(const Vector3Df& b) const;
    Vector3Df subtract(const Vector3Df& b) const;

    Vector3Df withX(float nx) const { return Vector3Df(nx, y_, z_); }
    Vector3Df withY(float ny) const { return Vector3Df(x_, ny, z_); }
    Vector3Df withZ(float nz) const { return Vector3Df(x_, y_, nz); }
    float x() const { return x_; }
    float y() const { return y_; }
    float z() const { return z_; }

    bool epsilonEquals(const Vector3Df& other) const { return epsilonEquals(other, 1e-6f); }
    bool epsilonEquals(const Vector3Df& other, float epsilon) const;

    bool operator==(const Vector3Df& other) const { return x_ == other.x_ && y_ == other.y_ && z_ == other.z_; }
};


#endif
