#include <cmath>

#include "vsdk/toolkit/common/color/ColorRgb.h"
ColorRgb::ColorRgb() : r_(0), g_(0), b_(0) {
}

ColorRgb::ColorRgb(const ColorRgb& c) : r_(c.r_), g_(c.g_), b_(c.b_) {
}

ColorRgb::ColorRgb(double r, double g, double b) : r_(r), g_(g), b_(b) {
}

ColorRgb::~ColorRgb() {
}

double ColorRgb::r() const {
    return r_;
}

double ColorRgb::g() const {
    return g_;
}

double ColorRgb::b() const {
    return b_;
}

double ColorRgb::getR() const {
    return r_;
}

double ColorRgb::getG() const {
    return g_;
}

double ColorRgb::getB() const {
    return b_;
}

double ColorRgb::distance(const ColorRgb& other) const {
    double dr = r_ - other.r_;
    double dg = g_ - other.g_;
    double db = b_ - other.b_;
    return std::sqrt(dr * dr + dg * dg + db * db);
}

ColorRgb ColorRgb::add(const ColorRgb& other) const {
    return ColorRgb(r_ + other.r_, g_ + other.g_, b_ + other.b_);
}

ColorRgb ColorRgb::multiply(double scalar) const {
    return ColorRgb(r_ * scalar, g_ * scalar, b_ * scalar);
}
