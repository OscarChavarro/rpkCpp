#include <cmath>
#include <cstring>

#include "java/lang/Double.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
const double Ray::UNIT_DIRECTION_TOLERANCE = 1e-12;

Ray::Ray() : Ray(Vector3Dd(0, 0, 0), Vector3Dd(1, 0, 0), 0.0) {}

Ray::Ray(const Vector3Dd& origin, const Vector3Dd& direction)
    : Ray(origin, direction, 0.0) {}

Ray::Ray(const Vector3Dd& origin, const Vector3Dd& direction, double t)
    : origin(origin), direction(normalizeDirection(direction)), t(t) {}

Ray Ray::fromAffine(const Vector3Dd& origin, const Vector3Dd& direction, double t)
{
    Ray result;
    result.setOriginAndDirection(origin, direction);
    result.setT(t);
    return result;
}

Ray Ray::copyOf(const Ray& other)
{
    return other;
}

Ray Ray::withOrigin(const Vector3Dd& newOrigin) const
{
    Ray result = *this;
    result.origin = newOrigin;
    return result;
}

Ray Ray::withDirection(const Vector3Dd& newDirection) const
{
    Ray result = *this;
    result.direction = newDirection;
    return result;
}

Ray Ray::withT(double newT) const
{
    Ray result = *this;
    result.t = newT;
    return result;
}

void Ray::setOrigin(const Vector3Dd& newOrigin)
{
    origin = newOrigin;
}

void Ray::setDirection(const Vector3Dd& newDirection)
{
    direction = newDirection;
}

void Ray::setT(double newT)
{
    t = newT;
}

void Ray::setOriginAndDirection(
    const Vector3Dd& newOrigin, const Vector3Dd& newDirection)
{
    origin = newOrigin;
    direction = newDirection;
}

Vector3Dd Ray::normalizeDirection(const Vector3Dd& direction)
{
    double lengthSquared = direction.dotProduct(direction);
    if ( lengthSquared <= VSDK::EPSILON ) {
        return direction;
    }
    if ( std::abs(lengthSquared - 1.0) <= UNIT_DIRECTION_TOLERANCE ) {
        return direction;
    }
    return direction.multiply(1.0 / std::sqrt(lengthSquared));
}

bool Ray::equals(const Ray& other) const
{
    return t == other.t && origin == other.origin && direction == other.direction;
}

static unsigned int hashDouble(double val) {
    unsigned char bytes[sizeof(double)];
    memcpy(bytes, &val, sizeof(double));
    unsigned int h = 0u;
    for (int i = 0; i < (int)sizeof(double); ++i)
        h = h * 31u + static_cast<unsigned int>(bytes[i]);
    return h;
}

int Ray::hashCode() const
{
    unsigned int result = hashDouble(t);
    result = 31u * result + static_cast<unsigned int>(origin.hashCode());
    result = 31u * result + static_cast<unsigned int>(direction.hashCode());
    return static_cast<int>(result);
}

/**
Provides an object to text report conversion, optimized for human
readability and debugging. Do not use for serialization or persistence
purposes.
@return human-readable representation of current Ray
*/
java::String Ray::toString() const
{
    return "Ray Origin: <" + VSDK::formatDouble(origin.x()) + ", " +
        VSDK::formatDouble(origin.y()) + ", " +
        VSDK::formatDouble(origin.z()) + ">; Direction: <" +
        VSDK::formatDouble(direction.x()) + ", " +
        VSDK::formatDouble(direction.y()) + ", " +
        VSDK::formatDouble(direction.z()) + "> T: " +
        VSDK::formatDouble(t);
}
