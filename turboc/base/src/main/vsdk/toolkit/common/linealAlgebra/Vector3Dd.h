#ifndef __VECTOR_3_DD__
#define __VECTOR_3_DD__


#include <math.h>
#include <string.h>

#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/Cpp98Support.h"
class Vector3Dd {
    double x_;
    double y_;
    double z_;

    static unsigned int hashDouble(double val) {
        unsigned char bytes[sizeof(double)];
        memcpy(bytes, &val, sizeof(double));
        unsigned int h = 0u;
        for (int i = 0; i < (int)sizeof(double); ++i)
            h = h * 31u + ((unsigned int)(bytes[i]));
        return h;
    }

public:
    inline Vector3Dd() : x_(0.0), y_(0.0), z_(0.0) {}
    inline Vector3Dd(double x, double y, double z) : x_(x), y_(y), z_(z) {}
    inline Vector3Dd(const Vector3Dd& other) : x_(other.x_), y_(other.y_), z_(other.z_) {}

    static inline Vector3Dd copyOf(const Vector3Dd& other) { return Vector3Dd(other); }

    inline Vector3Dd multiply(double a) const { return Vector3Dd(a * x_, a * y_, a * z_); }
    inline Vector3Dd multiply(const Vector3Dd& other) const
    {
        return Vector3Dd(x_ * other.x_, y_ * other.y_, z_ * other.z_);
    }
    inline Vector3Dd midpoint(const Vector3Dd& other) const
    {
        return Vector3Dd(
            0.5 * (x_ + other.x_), 0.5 * (y_ + other.y_), 0.5 * (z_ + other.z_));
    }
    inline Vector3Dd crossProduct(const Vector3Dd& other) const
    {
        return Vector3Dd(
            y_ * other.z_ - z_ * other.y_, z_ * other.x_ - x_ * other.z_,
            x_ * other.y_ - y_ * other.x_);
    }
    inline double dotProduct(const Vector3Dd& other) const
    {
        return x_ * other.x_ + y_ * other.y_ + z_ * other.z_;
    }
    inline Vector3Dd normalized() const
    {
        double t = x_ * x_ + y_ * y_ + z_ * z_;
        if (Cpp98Support::abs(t) < VSDK::EPSILON) {
            return *this;
        }
        if (t != 0.0 && t != 1.0) {
            t = 1.0 / sqrt(t);
        }
        return Vector3Dd(x_ * t, y_ * t, z_ * t);
    }
    inline Vector3Dd normalizedFast() const
    {
        const double vTemp = sqrt(x_*x_ + y_*y_ + z_*z_);
        return Vector3Dd(x_ / vTemp, y_ / vTemp, z_ / vTemp);
    }
    inline double length() const { return sqrt(x_ * x_ + y_ * y_ + z_ * z_); }
    inline Vector3Dd add(const Vector3Dd& b) const
    {
        return Vector3Dd(x_ + b.x_, y_ + b.y_, z_ + b.z_);
    }
    inline Vector3Dd subtract(const Vector3Dd& b) const
    {
        return Vector3Dd(x_ - b.x_, y_ - b.y_, z_ - b.z_);
    }
    float* exportToFloatArrayVector() const;
    double obtainSphericalThetaAngle() const;
    double obtainSphericalPhiAngle() const;
    static Vector3Dd fromSpherical(double r, double theta, double phi);

    Vector3Dd withX(double nx) const { return Vector3Dd(nx, y_, z_); }
    Vector3Dd withY(double ny) const { return Vector3Dd(x_, ny, z_); }
    Vector3Dd withZ(double nz) const { return Vector3Dd(x_, y_, nz); }
    double x() const { return x_; }
    double y() const { return y_; }
    double z() const { return z_; }

    BOOLEAN epsilonEquals(const Vector3Dd& other) const { return epsilonEquals(other, 1e-6); }
    BOOLEAN epsilonEquals(const Vector3Dd& other, double epsilon) const;

    BOOLEAN operator==(const Vector3Dd& other) const { return x_ == other.x_ && y_ == other.y_ && z_ == other.z_; }
    BOOLEAN equals(const Vector3Dd& other) const { return (*this) == other; }
    int hashCode() const {
        unsigned int result = hashDouble(x_);
        result = 31u * result + hashDouble(y_);
        result = 31u * result + hashDouble(z_);
        return (int)result;
    }
    String* toString() const;
};

/** Equality of the elements of ArrayList (see vsdkEquals in ArrayList.h) */
inline BOOLEAN
vsdkEquals(const Vector3Dd &a, const Vector3Dd &b)
{
    return a == b;
}


#endif
