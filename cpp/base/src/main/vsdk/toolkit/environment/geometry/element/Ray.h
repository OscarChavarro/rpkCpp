#ifndef __RAY__
#define __RAY__

#include "java/lang/String.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
/**
 This class models a mathematical RAY.
 */
class Ray {
private:
    static const double UNIT_DIRECTION_TOLERANCE;

    Vector3Dd origin;
    Vector3Dd direction;
    double t;

    static Vector3Dd normalizeDirection(const Vector3Dd& direction);

public:
    Ray();
    Ray(const Vector3Dd& origin, const Vector3Dd& direction);
    Ray(const Vector3Dd& origin, const Vector3Dd& direction, double t);

    // Explicit affine construction: preserve direction magnitude and t.
    // Legacy constructors retain their historical normalization behavior.
    static Ray fromAffine(const Vector3Dd& origin, const Vector3Dd& direction, double t = 0.0);
    static Ray copyOf(const Ray& other);

    // Copies replace only the named component; they never renormalize.
    Ray withOrigin(const Vector3Dd& newOrigin) const;
    Ray withDirection(const Vector3Dd& newDirection) const;
    Ray withT(double newT) const;

    const Vector3Dd& getOrigin() const;
    const Vector3Dd& getDirection() const;
    double getT() const;
    void setOrigin(const Vector3Dd& newOrigin);
    void setDirection(const Vector3Dd& newDirection);
    void setT(double newT);
    void setOriginAndDirection(
        const Vector3Dd& newOrigin, const Vector3Dd& newDirection);

    bool equals(const Ray& other) const;
    int hashCode() const;
    java::String toString() const;
};

inline const Vector3Dd& Ray::getOrigin() const
{
    return origin;
}

inline const Vector3Dd& Ray::getDirection() const
{
    return direction;
}

inline double Ray::getT() const
{
    return t;
}

#endif
