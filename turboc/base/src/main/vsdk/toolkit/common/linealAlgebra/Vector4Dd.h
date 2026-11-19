#ifndef __VECTOR_4_DD__
#define __VECTOR_4_DD__


#include <math.h>
#include <string.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"
class Vector3Dd;

class Vector4Dd {
    double x_;
    double y_;
    double z_;
    double w_;

    static unsigned int hashDouble(double val) {
        unsigned char bytes[sizeof(double)];
        memcpy(bytes, &val, sizeof(double));
        unsigned int h = 0u;
        for (int i = 0; i < (int)sizeof(double); ++i)
            h = h * 31u + ((unsigned int)(bytes[i]));
        return h;
    }

public:
    /// Origin (0, 0, 0, 0), so containers can hold values of this class
    Vector4Dd() : x_(0), y_(0), z_(0), w_(0) {}
    Vector4Dd(double x, double y, double z, double w) : x_(x), y_(y), z_(z), w_(w) {}
    Vector4Dd(const Vector4Dd& other) : x_(other.x_), y_(other.y_), z_(other.z_), w_(other.w_) {}
    explicit Vector4Dd(const Vector3Dd& other);

    Vector4Dd multiply(double a) const;
    Vector4Dd dividedByW() const;
    double length() const;
    Vector4Dd add(const Vector4Dd& b) const;

    Vector4Dd withX(double nx) const { return Vector4Dd(nx, y_, z_, w_); }
    Vector4Dd withY(double ny) const { return Vector4Dd(x_, ny, z_, w_); }
    Vector4Dd withZ(double nz) const { return Vector4Dd(x_, y_, nz, w_); }
    Vector4Dd withW(double nw) const { return Vector4Dd(x_, y_, z_, nw); }
    double x() const { return x_; }
    double y() const { return y_; }
    double z() const { return z_; }
    double w() const { return w_; }

    BOOLEAN epsilonEquals(const Vector4Dd& other) const { return epsilonEquals(other, 1e-6); }
    BOOLEAN epsilonEquals(const Vector4Dd& other, double epsilon) const;

    BOOLEAN operator==(const Vector4Dd& other) const { return x_ == other.x_ && y_ == other.y_ && z_ == other.z_ && w_ == other.w_; }
    BOOLEAN equals(const Vector4Dd& other) const { return (*this) == other; }
    int hashCode() const {
        unsigned int result = hashDouble(x_);
        result = 31u * result + hashDouble(y_);
        result = 31u * result + hashDouble(z_);
        result = 31u * result + hashDouble(w_);
        return (int)result;
    }
    String* toString() const;
};

/** Equality of the elements of ArrayList (see vsdkEquals in ArrayList.h) */
inline BOOLEAN
vsdkEquals(const Vector4Dd &a, const Vector4Dd &b)
{
    return a == b;
}


#endif
