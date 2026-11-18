#ifndef __VECTOR_2_DF__
#define __VECTOR_2_DF__


#include <cmath>

#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/common/logging/Logger.h"
class Vector2Dd;

class Vector2Df {
    float x_;
    float y_;

public:
    Vector2Df() : x_(0.0f), y_(0.0f) {}
    Vector2Df(float x, float y) : x_(x), y_(y) {}
    Vector2Df(const Vector2Df& other) : x_(other.x_), y_(other.y_) {}
    explicit Vector2Df(const Vector2Dd& other);

    Vector2Df multiply(float a) const { return Vector2Df(a * x_, a * y_); }
    float length() const { return std::sqrt(x_ * x_ + y_ * y_); }
    Vector2Df add(const Vector2Df& b) const { return Vector2Df(x_ + b.x_, y_ + b.y_); }

    Vector2Df withX(float nx) const { return Vector2Df(nx, y_); }
    Vector2Df withY(float ny) const { return Vector2Df(x_, ny); }
    float x() const { return x_; }
    float y() const { return y_; }

    bool epsilonEquals(const Vector2Df& other) const { return epsilonEquals(other, 1e-6f); }
    bool epsilonEquals(const Vector2Df& other, float epsilon) const {
        if ( epsilon < 0.0f ) {
            Logger::reportMessage("Vector2Df", Logger::ERROR, "epsilonEquals", "epsilon must be >= 0");
            throw VSDKFatalException("epsilon must be >= 0");
        }
        return std::abs(x_ - other.x_) <= epsilon && std::abs(y_ - other.y_) <= epsilon;
    }

    bool operator==(const Vector2Df& other) const { return x_ == other.x_ && y_ == other.y_; }
};


#endif
