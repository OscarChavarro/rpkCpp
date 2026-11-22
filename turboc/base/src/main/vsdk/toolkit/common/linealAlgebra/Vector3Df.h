#ifndef __VECTOR_3_DF__
#define __VECTOR_3_DF__


#include <math.h>
#include "vsdk/toolkit/common/VSDK.h"
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
    Vector3Df& operator=(const Vector3Df& other) { x_ = other.x_; y_ = other.y_; z_ = other.z_; return *this; }

    // The element-wise operations are inline: they sit in the inner loops of
    // ray casting and radiosity clients, where a call per operation costs
    Vector3Df multiply(float a) const { return Vector3Df(a * x_, a * y_, a * z_); }
    Vector3Df crossProduct(const Vector3Df& other) const {
        return Vector3Df(y_ * other.z_ - z_ * other.y_, z_ * other.x_ - x_ * other.z_, x_ * other.y_ - y_ * other.x_);
    }
    float dotProduct(const Vector3Df& other) const { return x_ * other.x_ + y_ * other.y_ + z_ * other.z_; }
    Vector3Df normalized() const;
    float length() const { return sqrt(x_ * x_ + y_ * y_ + z_ * z_); }
    Vector3Df add(const Vector3Df& b) const { return Vector3Df(x_ + b.x_, y_ + b.y_, z_ + b.z_); }
    Vector3Df subtract(const Vector3Df& b) const { return Vector3Df(x_ - b.x_, y_ - b.y_, z_ - b.z_); }

    /**
    Square of the length (the dot product with itself)
    */
    float lengthSquared() const { return x_ * x_ + y_ * y_ + z_ * z_; }

    /**
    Sum of the absolute values of the components (the L1 norm)
    */
    float manhattanLength() const { return absolute(x_) + absolute(y_) + absolute(z_); }

    /**
    Distance to the given point: the length of (other - this)
    */
    float distance(const Vector3Df& other) const { return other.subtract(*this).length(); }

    /**
    Square of the distance to the given point
    */
    float distanceSquared(const Vector3Df& other) const { return other.subtract(*this).lengthSquared(); }

    /**
    Scales by 1 / s, or returns this vector unchanged when s is within
    [-epsilon, epsilon]. The factor 1 / s is computed once and multiplies
    each component
    */
    Vector3Df multiplyByInverse(float s, float epsilon) const {
        const float factor = s < -epsilon || s > epsilon ? 1.0f / s : 1.0f;
        return Vector3Df(factor * x_, factor * y_, factor * z_);
    }

    /**
    Scales by the inverse of the length, or returns this vector unchanged
    when the length is within [-epsilon, epsilon]. Unlike normalized(), the
    threshold applies to the length, and a unit vector is also rescaled
    */
    Vector3Df normalized(float epsilon) const { return multiplyByInverse(length(), epsilon); }

    /**
    Index of the component of largest absolute value: 0 for x, 1 for y,
    2 for z; ties go to x before y and y before z
    */
    int dominantAxis() const {
        const float ax = absolute(x_);
        const float ay = absolute(y_);
        const float az = absolute(z_);
        float m = ay > az ? ay : az;
        m = ax > m ? ax : m;
        if ( m == ax ) {
            return 0;
        }
        return m == ay ? 1 : 2;
    }

    Vector3Df withX(float nx) const { return Vector3Df(nx, y_, z_); }
    Vector3Df withY(float ny) const { return Vector3Df(x_, ny, z_); }
    Vector3Df withZ(float nz) const { return Vector3Df(x_, y_, nz); }
    float x() const { return x_; }
    float y() const { return y_; }
    float z() const { return z_; }

    BOOLEAN epsilonEquals(const Vector3Df& other) const { return epsilonEquals(other, 1e-6f); }
    BOOLEAN epsilonEquals(const Vector3Df& other, float epsilon) const;

    BOOLEAN operator==(const Vector3Df& other) const { return x_ == other.x_ && y_ == other.y_ && z_ == other.z_; }

private:
    static float absolute(float a) { return a < 0.0f ? -a : a; }
};

/** Equality of the elements of ArrayList (see vsdkEquals in ArrayList.h) */
inline BOOLEAN
vsdkEquals(const Vector3Df &a, const Vector3Df &b)
{
    return a == b;
}


#endif
