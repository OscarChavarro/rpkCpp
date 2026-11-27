#include <math.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"

double ColorRgb::distance(const ColorRgb& other) const {
    double dr = r_ - other.r_;
    double dg = g_ - other.g_;
    double db = b_ - other.b_;
    return sqrt(dr * dr + dg * dg + db * db);
}

BOOLEAN ColorRgb::isBlack() const {
    return r_ > -VSDK::EPSILON && r_ < VSDK::EPSILON &&
           g_ > -VSDK::EPSILON && g_ < VSDK::EPSILON &&
           b_ > -VSDK::EPSILON && b_ < VSDK::EPSILON;
}
