#ifndef __VECTOR_3_DD__
#define __VECTOR_3_DD__


#include <cmath>
#include <cstring>

#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDK.h"
class Vector3Dd {
    double x_;
    double y_;
    double z_;

    static unsigned int hashDouble(double val) {
        unsigned char bytes[sizeof(double)];
        memcpy(bytes, &val, sizeof(double));
        unsigned int h = 0u;
        for (int i = 0; i < (int)sizeof(double); ++i)
            h = h * 31u + static_cast<unsigned int>(bytes[i]);
        return h;
    }

public:
    inline Vector3Dd() : x_(0.0), y_(0.0), z_(0.0) {}
    inline Vector3Dd(double x, double y, double z) : x_(x), y_(y), z_(z) {}
    inline Vector3Dd(const Vector3Dd& other) : x_(other.x_), y_(other.y_), z_(other.z_) {}
    inline Vector3Dd& operator=(const Vector3Dd& other)
    {
        x_ = other.x_;
        y_ = other.y_;
        z_ = other.z_;
        return *this;
    }

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
        if (std::abs(t) < VSDK::EPSILON) {
            return *this;
        }
        if (t != 0.0 && t != 1.0) {
            t = 1.0 / std::sqrt(t);
        }
        return Vector3Dd(x_ * t, y_ * t, z_ * t);
    }
    inline Vector3Dd normalizedFast() const
    {
        const double vTemp = std::sqrt(x_*x_ + y_*y_ + z_*z_);
        return Vector3Dd(x_ / vTemp, y_ / vTemp, z_ / vTemp);
    }
    inline double length() const { return std::sqrt(x_ * x_ + y_ * y_ + z_ * z_); }
    static inline double distance(const Vector3Dd& a, const Vector3Dd& b)
    {
        return std::sqrt((a.x_ - b.x_)*(a.x_ - b.x_) +
                         (a.y_ - b.y_)*(a.y_ - b.y_) +
                         (a.z_ - b.z_)*(a.z_ - b.z_));
    }
    inline Vector3Dd add(const Vector3Dd& b) const
    {
        return Vector3Dd(x_ + b.x_, y_ + b.y_, z_ + b.z_);
    }
    inline Vector3Dd subtract(const Vector3Dd& b) const
    {
        return Vector3Dd(x_ - b.x_, y_ - b.y_, z_ - b.z_);
    }

    /**
    Square of the length (the dot product with itself)
    */
    inline double lengthSquared() const { return x_ * x_ + y_ * y_ + z_ * z_; }

    /**
    Sum of the absolute values of the components (the L1 norm)
    */
    inline double manhattanLength() const { return absolute(x_) + absolute(y_) + absolute(z_); }

    /**
    Distance to the given point: the length of (other - this)
    */
    inline double distance(const Vector3Dd& other) const { return other.subtract(*this).length(); }

    /**
    Square of the distance to the given point
    */
    inline double distanceSquared(const Vector3Dd& other) const { return other.subtract(*this).lengthSquared(); }

    /**
    Scales by 1 / s, or returns this vector unchanged when s is within
    [-epsilon, epsilon]. The factor 1 / s is computed once and multiplies
    each component
    */
    inline Vector3Dd multiplyByInverse(double s, double epsilon) const
    {
        const double factor = s < -epsilon || s > epsilon ? 1.0 / s : 1.0;
        return Vector3Dd(factor * x_, factor * y_, factor * z_);
    }

    /**
    Scales by the inverse of the length, or returns this vector unchanged
    when the length is within [-epsilon, epsilon]. Unlike normalized(), the
    threshold applies to the length, and a unit vector is also rescaled
    */
    inline Vector3Dd normalized(double epsilon) const { return multiplyByInverse(length(), epsilon); }

    /**
    Index of the component of largest absolute value: 0 for x, 1 for y,
    2 for z; ties go to x before y and y before z
    */
    inline int dominantAxis() const
    {
        const double ax = absolute(x_);
        const double ay = absolute(y_);
        const double az = absolute(z_);
        double m = ay > az ? ay : az;
        m = ax > m ? ax : m;
        if ( m == ax ) {
            return 0;
        }
        return m == ay ? 1 : 2;
    }

    /**
    The component of the given axis: 0 for x, 1 for y, 2 for z
    */
    inline double component(int axis) const { return axis == 0 ? x_ : (axis == 1 ? y_ : z_); }

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

    bool epsilonEquals(const Vector3Dd& other) const { return epsilonEquals(other, 1e-6); }
    bool epsilonEquals(const Vector3Dd& other, double epsilon) const;

    bool operator==(const Vector3Dd& other) const { return x_ == other.x_ && y_ == other.y_ && z_ == other.z_; }
    bool equals(const Vector3Dd& other) const { return (*this) == other; }
    int hashCode() const {
        unsigned int result = hashDouble(x_);
        result = 31u * result + hashDouble(y_);
        result = 31u * result + hashDouble(z_);
        return (int)result;
    }
    java::String* toString() const;

private:
    static inline double absolute(double a) { return a < 0.0 ? -a : a; }
};


#endif
