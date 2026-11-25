#ifndef __COLOR_RGB__
#define __COLOR_RGB__

#include "java/lang/Math.h"

/**
Immutable RGB color. Every arithmetic operation returns a new color and
leaves this one untouched. Component expressions are evaluated in a fixed
left-to-right order so results are bit-identical across ports.
*/
class ColorRgb {
private:
    double r_;
    double g_;
    double b_;

public:
    ColorRgb() : r_(0), g_(0), b_(0) {}
    ColorRgb(double r, double g, double b) : r_(r), g_(g), b_(b) {}
    ~ColorRgb() {}

    double r() const { return r_; }
    double g() const { return g_; }
    double b() const { return b_; }

    double getR() const { return r_; }
    double getG() const { return g_; }
    double getB() const { return b_; }

    double distance(const ColorRgb& other) const;

    /** Component-wise this + other. */
    ColorRgb add(const ColorRgb& other) const { return ColorRgb(r_ + other.r_, g_ + other.g_, b_ + other.b_); }

    /** Component-wise this + scalar * other. */
    ColorRgb addScaled(double scalar, const ColorRgb& other) const {
        return ColorRgb(r_ + scalar * other.r_, g_ + scalar * other.g_, b_ + scalar * other.b_);
    }

    /** Component-wise this - other. */
    ColorRgb subtract(const ColorRgb& other) const { return ColorRgb(r_ - other.r_, g_ - other.g_, b_ - other.b_); }

    /** this * scalar. */
    ColorRgb multiply(double scalar) const { return ColorRgb(r_ * scalar, g_ * scalar, b_ * scalar); }

    /** Component-wise (Hadamard) product this * other. */
    ColorRgb multiply(const ColorRgb& other) const { return ColorRgb(r_ * other.r_, g_ * other.g_, b_ * other.b_); }

    /**
    Multiplies by the reciprocal of scalar (not a true division: the
    reciprocal is computed once). A zero scalar returns this color unchanged.
    */
    ColorRgb scaleInverse(double scalar) const {
        const double a = scalar != 0.0 ? 1.0 / scalar : 1.0;
        return ColorRgb(a * r_, a * g_, a * b_);
    }

    /**
    Component-wise this / other. A component whose divisor is zero is
    returned unchanged.
    */
    ColorRgb divide(const ColorRgb& other) const {
        return ColorRgb(other.r_ != 0.0 ? r_ / other.r_ : r_,
                        other.g_ != 0.0 ? g_ / other.g_ : g_,
                        other.b_ != 0.0 ? b_ / other.b_ : b_);
    }

    /** Component-wise absolute value. */
    ColorRgb absolute() const {
        return ColorRgb(java::Math::abs(r_), java::Math::abs(g_), java::Math::abs(b_));
    }

    /** Component-wise minimum. */
    ColorRgb minimum(const ColorRgb& other) const {
        return ColorRgb(r_ < other.r_ ? r_ : other.r_,
                        g_ < other.g_ ? g_ : other.g_,
                        b_ < other.b_ ? b_ : other.b_);
    }

    /** Component-wise maximum. */
    ColorRgb maximum(const ColorRgb& other) const {
        return ColorRgb(r_ > other.r_ ? r_ : other.r_,
                        g_ > other.g_ ? g_ : other.g_,
                        b_ > other.b_ ? b_ : other.b_);
    }

    /** |r| + |g| + |b|. */
    double sumAbsComponents() const {
        return java::Math::abs(r_) + java::Math::abs(g_) + java::Math::abs(b_);
    }

    /** Largest of the three components. */
    double maximumComponent() const {
        return r_ > g_ ? (r_ > b_ ? r_ : b_) : (g_ > b_ ? g_ : b_);
    }

    /** (r + g + b) / 3. */
    double average() const { return (r_ + g_ + b_) / 3.0; }

    /** True when every component lies strictly inside (-VSDK::EPSILON, VSDK::EPSILON). */
    bool isBlack() const;
};

#endif
