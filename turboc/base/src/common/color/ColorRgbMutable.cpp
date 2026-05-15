#include "java/lang/Math.h"
#include "common/linealAlgebra/Numeric.h"
#include "common/color/ColorRgbMutable.h"

ColorRgbMutable::ColorRgbMutable(const float inR, const float inG, const float inB) {
    r = inR;
    g = inG;
    b = inB;
}

void
ColorRgbMutable::abs() {
    r = Math::abs(r);
    g = Math::abs(g);
    b = Math::abs(b);
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
ColorRgbMutable::addConstant(const ColorRgbMutable s, const float a) {
    r = s.r + a;
    g = s.g + a;
    b = s.b + a;
}

void
ColorRgbMutable::subtract(const ColorRgbMutable s, const ColorRgbMutable t) {
    r = s.r - t.r;
    g = s.g - t.g;
    b = s.b - t.b;
}

void
ColorRgbMutable::scaleInverse(const float scale, const ColorRgbMutable s) {
    float a = (scale != 0.0f) ? 1.0f / scale : 1.0f;
    r = a * s.r;
    g = a * s.g;
    b = a * s.b;
}

float
ColorRgbMutable::sumAbsComponents() const {
    return Math::abs(r) + Math::abs(g) + Math::abs(b);
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

float
ColorRgbMutable::average() const {
    return (r + g + b) / 3.0f;
}

void
ColorRgbMutable::interpolateBarycentric(const ColorRgbMutable c0, const ColorRgbMutable c1, const ColorRgbMutable c2, const float u, const float v) {
    r = c0.r + u * (c1.r - c0.r) + v * (c2.r - c0.r);
    g = c0.g + u * (c1.g - c0.g) + v * (c2.g - c0.g);
    b = c0.b + u * (c1.b - c0.b) + v * (c2.b - c0.b);
}

void
ColorRgbMutable::interpolateBiLinear(const ColorRgbMutable c0, const ColorRgbMutable c1, const ColorRgbMutable c2, const ColorRgbMutable c3, const float u, const float v) {
    float c = u * v;
    float bb = u - c;
    float d = v - c;

    r = c0.r + bb * (c1.r - c0.r) + c * (c2.r - c0.r) + d * (c3.r - c0.r);
    g = c0.g + bb * (c1.g - c0.g) + c * (c2.g - c0.g) + d * (c3.g - c0.g);
    b = c0.b + bb * (c1.b - c0.b) + c * (c2.b - c0.b) + d * (c3.b - c0.b);
}

void
ColorRgbMutable::clip() {
    if ( r < 0.0f ) {
        r = 0.0f;
    } else {
        r = r > 1.0f ? 1.0f : r;
    }

    if ( g < 0.0f ) {
        g = 0.0f;
    } else {
        g = g > 1.0f ? 1.0f : g;
    }

    if ( b < 0.0f ) {
        b = 0.0f;
    } else {
        b = b > 1.0f ? 1.0f : b;
    }
}

void
ColorRgbMutable::print(PrintStream *stream) const {
    if ( stream == NULL ) {
        return;
    }
    stream->printf("%g %g %g", r, g, b);
}
