#ifndef __QUATERNIONF__
#define __QUATERNIONF__


#include "java/lang/String.h"
#include "vsdk/toolkit/common/linealAlgebra/Quaterniond.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
class Quaternionf {
    Vector3Df direction_;
    float magnitude_;

public:
    Quaternionf() : direction_(Vector3Df(0.0f, 0.0f, 0.0f)), magnitude_(0.0f) {}
    Quaternionf(const Vector3Df& direction, float magnitude) : direction_(direction), magnitude_(magnitude) {}
    Quaternionf(const Quaternionf& other) : direction_(other.direction_), magnitude_(other.magnitude_) {}
    explicit Quaternionf(const Quaterniond& other);

    static Quaternionf copyOf(const Quaternionf& other) { return Quaternionf(other); }

    float lengthSquared() const;
    float length() const;
    Quaternionf normalized() const;
    Quaternionf conjugated() const;
    Vector3Df rotate(const Vector3Df& vector) const;

    Quaternionf withDirection(const Vector3Df& newDirection) const { return Quaternionf(newDirection, magnitude_); }
    Quaternionf withMagnitude(float newMagnitude) const { return Quaternionf(direction_, newMagnitude); }

    Vector3Df direction() const { return direction_; }
    float magnitude() const { return magnitude_; }

    bool epsilonEquals(const Quaternionf& other) const { return epsilonEquals(other, 1e-6f); }
    bool epsilonEquals(const Quaternionf& other, float epsilon) const;
    java::String* toString() const;
};


#endif
