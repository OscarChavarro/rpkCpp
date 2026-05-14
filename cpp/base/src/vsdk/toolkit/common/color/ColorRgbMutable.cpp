#include "vsdk/toolkit/java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"

ColorRgbMutable::ColorRgbMutable(const double inR, const double inG, const double inB) {
    r = inR;
    g = inG;
    b = inB;
}

void
ColorRgbMutable::abs() {
    r = java::Math::abs(r);
    g = java::Math::abs(g);
    b = java::Math::abs(b);
}

void
ColorRgbMutable::arrayCopy(ColorRgbMutable *result, const ColorRgbMutable *source, const char n) {
    for ( int i = 0; i < n; i++ ) {
        result[i] = source[i];
    }
}

void
ColorRgbMutable::arrayAdd(ColorRgbMutable *result, const ColorRgbMutable *source, const char n) {
    for ( int i = 0; i < n; i++ ) {
        result[i].add(result[i], source[i]);
    }
}

void
ColorRgbMutable::arrayClear(ColorRgbMutable *color, const char n) {
    for ( int i = 0; i < n; i++ ) {
        color[i].clear();
    }
}

bool
ColorRgbMutable::isBlack() const {
    return (r > -Numeric::EPSILON && r < Numeric::EPSILON &&
            g > -Numeric::EPSILON && g < Numeric::EPSILON &&
            b > -Numeric::EPSILON && b < Numeric::EPSILON);
}

void
ColorRgbMutable::scalarProduct(const ColorRgbMutable s, const ColorRgbMutable t) {
    r = s.r * t.r;
    g = s.g * t.g;
    b = s.b * t.b;
}

void
ColorRgbMutable::selfScalarProduct(const ColorRgbMutable s) {
    r *= s.r;
    g *= s.g;
    b *= s.b;
}

void
ColorRgbMutable::add(const ColorRgbMutable s, const ColorRgbMutable t) {
    r = s.r + t.r;
    g = s.g + t.g;
    b = s.b + t.b;
}

void
ColorRgbMutable::addConstant(const ColorRgbMutable s, const double a) {
    r = s.r + a;
    g = s.g + a;
    b = s.b + a;
}

void
ColorRgbMutable::subtract(const ColorRgbMutable s, const ColorRgbMutable  t) {
    r = s.r - t.r;
    g = s.g - t.g;
    b = s.b - t.b;
}

void
ColorRgbMutable::scaleInverse(const double scale, const ColorRgbMutable s) {
    const double a = (scale != 0.0) ? 1.0 / scale : 1.0;
    r = a * s.r;
    g = a * s.g;
    b = a * s.b;
}

double
ColorRgbMutable::sumAbsComponents() const {
    return java::Math::abs(r) + java::Math::abs(g) + java::Math::abs(b);
}

void
ColorRgbMutable::maximum(const ColorRgbMutable s, const ColorRgbMutable t) {
    r = s.r > t.r ? s.r : t.r;
    g = s.g > t.g ? s.g : t.g;
    b = s.b > t.b ? s.b : t.b;
}

void
ColorRgbMutable::minimum(const ColorRgbMutable s, const ColorRgbMutable t) {
    r = s.r < t.r ? s.r : t.r;
    g = s.g < t.g ? s.g : t.g;
    b = s.b < t.b ? s.b : t.b;
}

double
ColorRgbMutable::average() const {
    return (r + g + b) / 3.0;
}

void
ColorRgbMutable::interpolateBarycentric(const ColorRgbMutable c0, const ColorRgbMutable c1, const ColorRgbMutable c2, const double u, const double v) {
    r = c0.r + u * (c1.r - c0.r) + v * (c2.r - c0.r);
    g = c0.g + u * (c1.g - c0.g) + v * (c2.g - c0.g);
    b = c0.b + u * (c1.b - c0.b) + v * (c2.b - c0.b);
}

void
ColorRgbMutable::interpolateBiLinear(const ColorRgbMutable c0, const ColorRgbMutable c1, const ColorRgbMutable c2, const ColorRgbMutable c3, const double u, const double v) {
    const double c = u * v;
    const double bb = u - c;
    const double d = v - c;

    r = c0.r + bb * (c1.r - c0.r) + c * (c2.r - c0.r) + d * (c3.r - c0.r);
    g = c0.g + bb * (c1.g - c0.g) + c * (c2.g - c0.g) + d * (c3.g - c0.g);
    b = c0.b + bb * (c1.b - c0.b) + c * (c2.b - c0.b) + d * (c3.b - c0.b);
}

void
ColorRgbMutable::clip() {
    if ( r < 0.0 ) {
        r = 0.0;
    } else {
        r = r > 1.0 ? 1.0 : r;
    }

    if ( g < 0.0 ) {
        g = 0.0;
    } else {
        g = g > 1.0 ? 1.0 : g;
    }

    if ( b < 0.0 ) {
        b = 0.0;
    } else {
        b = b > 1.0 ? 1.0 : b;
    }
}

void
ColorRgbMutable::print(java::PrintStream *stream) const {
    if ( stream == nullptr ) {
        return;
    }
    stream->printf("%g %g %g", r, g, b);
}
