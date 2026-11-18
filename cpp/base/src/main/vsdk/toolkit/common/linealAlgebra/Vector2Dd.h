#ifndef __VECTOR_2_DD__
#define __VECTOR_2_DD__


#include <cmath>
#include <cstring>

#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/common/logging/Logger.h"
class Vector2Dd {
public:
    double x;
    double y;

private:
    static unsigned int hashDouble(double val) {
        unsigned char bytes[sizeof(double)];
        memcpy(bytes, &val, sizeof(double));
        unsigned int h = 0u;
        for (int i = 0; i < (int)sizeof(double); ++i)
            h = h * 31u + static_cast<unsigned int>(bytes[i]);
        return h;
    }

public:
    Vector2Dd() : x(0.0), y(0.0) {}
    Vector2Dd(double x, double y) : x(x), y(y) {}
    Vector2Dd(const Vector2Dd& other) : x(other.x), y(other.y) {}

    static Vector2Dd copyOf(const Vector2Dd& other) { return Vector2Dd(other); }

    Vector2Dd multiply(double a) const { return Vector2Dd(a * x, a * y); }
    double length() const { return std::sqrt(x * x + y * y); }
    Vector2Dd add(const Vector2Dd& b) const { return Vector2Dd(x + b.x, y + b.y); }

    Vector2Dd withX(double nx) const { return Vector2Dd(nx, y); }
    Vector2Dd withY(double ny) const { return Vector2Dd(x, ny); }

    bool epsilonEquals(const Vector2Dd& other) const { return epsilonEquals(other, 1e-6); }
    bool epsilonEquals(const Vector2Dd& other, double epsilon) const {
        if ( epsilon < 0.0 ) {
            Logger::reportMessage("Vector2Dd", Logger::ERROR, "epsilonEquals", "epsilon must be >= 0");
            throw VSDKFatalException("epsilon must be >= 0");
        }
        return std::abs(x - other.x) <= epsilon && std::abs(y - other.y) <= epsilon;
    }

    bool operator==(const Vector2Dd& other) const { return x == other.x && y == other.y; }
    bool equals(const Vector2Dd& other) const { return (*this) == other; }
    int hashCode() const {
        unsigned int result = hashDouble(x);
        result = 31u * result + hashDouble(y);
        return (int)result;
    }
    java::String* toString() const;
};


#endif
