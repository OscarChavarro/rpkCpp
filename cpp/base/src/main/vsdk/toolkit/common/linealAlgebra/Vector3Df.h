#ifndef __VECTOR_3_DF__
#define __VECTOR_3_DF__


#include <cmath>
class Vector3Dd;

class Vector3Df {
    float components[3];

public:
    Vector3Df() : components{0.0f, 0.0f, 0.0f} {}
    Vector3Df(float x, float y, float z) : components{x, y, z} {}
    Vector3Df(const Vector3Df& other) : components{other.x(), other.y(), other.z()} {}
    explicit Vector3Df(const Vector3Dd& other);
    Vector3Df& operator=(const Vector3Df& other) = default;

    // The element-wise operations are inline: they sit in the inner loops of
    // ray casting and radiosity clients, where a call per operation costs
    Vector3Df multiply(float a) const { return Vector3Df(a * components[0], a * components[1], a * components[2]); }
    Vector3Df crossProduct(const Vector3Df& other) const {
        return Vector3Df(
            components[1] * other.components[2] - components[2] * other.components[1],
            components[2] * other.components[0] - components[0] * other.components[2],
            components[0] * other.components[1] - components[1] * other.components[0]);
    }
    float dotProduct(const Vector3Df& other) const { return components[0] * other.components[0] + components[1] * other.components[1] + components[2] * other.components[2]; }
    Vector3Df normalized() const;
    float length() const { return std::sqrt(components[0] * components[0] + components[1] * components[1] + components[2] * components[2]); }
    Vector3Df add(const Vector3Df& b) const { return Vector3Df(components[0] + b.components[0], components[1] + b.components[1], components[2] + b.components[2]); }
    Vector3Df subtract(const Vector3Df& b) const { return Vector3Df(components[0] - b.components[0], components[1] - b.components[1], components[2] - b.components[2]); }

    /**
    Square of the length (the dot product with itself)
    */
    float lengthSquared() const { return components[0] * components[0] + components[1] * components[1] + components[2] * components[2]; }

    /**
    Sum of the absolute values of the components (the L1 norm)
    */
    float manhattanLength() const { return absolute(components[0]) + absolute(components[1]) + absolute(components[2]); }

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
        return Vector3Df(factor * components[0], factor * components[1], factor * components[2]);
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
        const float ax = absolute(components[0]);
        const float ay = absolute(components[1]);
        const float az = absolute(components[2]);
        float m = ay > az ? ay : az;
        m = ax > m ? ax : m;
        if ( m == ax ) {
            return 0;
        }
        return m == ay ? 1 : 2;
    }

    Vector3Df withX(float nx) const { return Vector3Df(nx, components[1], components[2]); }
    Vector3Df withY(float ny) const { return Vector3Df(components[0], ny, components[2]); }
    Vector3Df withZ(float nz) const { return Vector3Df(components[0], components[1], nz); }
    float component(int axis) const { return components[axis]; }
    float x() const { return components[0]; }
    float y() const { return components[1]; }
    float z() const { return components[2]; }

    bool epsilonEquals(const Vector3Df& other) const { return epsilonEquals(other, 1e-6f); }
    bool epsilonEquals(const Vector3Df& other, float epsilon) const;

    bool operator==(const Vector3Df& other) const { return components[0] == other.components[0] && components[1] == other.components[1] && components[2] == other.components[2]; }

private:
    static float absolute(float a) { return a < 0.0f ? -a : a; }
};


#endif
