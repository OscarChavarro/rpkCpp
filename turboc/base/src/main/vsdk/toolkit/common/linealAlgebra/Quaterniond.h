#ifndef __QUATERNIOND__
#define __QUATERNIOND__


#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class Quaterniond {
    Vector3Dd direction_;
    double magnitude_;

public:
    Quaterniond() : direction_(Vector3Dd(0.0, 0.0, 0.0)), magnitude_(0.0) {}
    Quaterniond(const Vector3Dd& direction, double magnitude) : direction_(direction), magnitude_(magnitude) {}
    Quaterniond(const Quaterniond& other) : direction_(other.direction_), magnitude_(other.magnitude_) {}

    static Quaterniond copyOf(const Quaterniond& other) { return Quaterniond(other); }

    double lengthSquared() const;
    double length() const;
    Quaterniond normalized() const;
    Quaterniond conjugated() const;
    Vector3Dd rotate(const Vector3Dd& vector) const;

    Quaterniond withDirection(const Vector3Dd& newDirection) const { return Quaterniond(newDirection, magnitude_); }
    Quaterniond withMagnitude(double newMagnitude) const { return Quaterniond(direction_, newMagnitude); }

    Vector3Dd direction() const { return direction_; }
    double magnitude() const { return magnitude_; }

    BOOLEAN epsilonEquals(const Quaterniond& other) const { return epsilonEquals(other, 1e-6); }
    BOOLEAN epsilonEquals(const Quaterniond& other, double epsilon) const;
    String* toString() const;
};


#endif
