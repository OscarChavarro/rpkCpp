#include "java/lang/Math.h"
#include "common/linealAlgebra/Numeric.h"
#include "common/color/ColorRgb.h"

ColorRgb::ColorRgb(const float inR, const float inG, const float inB) {
    r = inR;
    g = inG;
    b = inB;
}

void
ColorRgb::abs() {
    r = java::Math::abs(r);
    g = java::Math::abs(g);
    b = java::Math::abs(b);
}

void
ColorRgb::arrayCopy(ColorRgb *result, const ColorRgb *source, const char n) {
    for ( int i = 0; i < n; i++ ) {
        result[i] = source[i];
    }
}

void
ColorRgb::arrayAdd(ColorRgb *result, const ColorRgb *source, const char n) {
    for ( int i = 0; i < n; i++ ) {
        result[i].add(result[i], source[i]);
    }
}

void
ColorRgb::arrayClear(ColorRgb *color, const char n) {
    for ( int i = 0; i < n; i++ ) {
        color[i].clear();
    }
}

bool
ColorRgb::isBlack() const {
    return (r > -Numeric::EPSILON && r < Numeric::EPSILON &&
            g > -Numeric::EPSILON && g < Numeric::EPSILON &&
            b > -Numeric::EPSILON && b < Numeric::EPSILON);
}

void
ColorRgb::scalarProduct(const ColorRgb s, const ColorRgb t) {
    r = s.r * t.r;
    g = s.g * t.g;
    b = s.b * t.b;
}

void
ColorRgb::selfScalarProduct(const ColorRgb s) {
    r *= s.r;
    g *= s.g;
    b *= s.b;
}

void
ColorRgb::add(const ColorRgb s, const ColorRgb t) {
    r = s.r + t.r;
    g = s.g + t.g;
    b = s.b + t.b;
}

void
ColorRgb::addConstant(const ColorRgb s, const float a) {
    r = s.r + a;
    g = s.g + a;
    b = s.b + a;
}

void
ColorRgb::subtract(const ColorRgb s, const ColorRgb  t) {
    r = s.r - t.r;
    g = s.g - t.g;
    b = s.b - t.b;
}

void
ColorRgb::scaleInverse(const float scale, const ColorRgb s) {
    const float a = (scale != 0.0F) ? 1.0F / scale : 1.0F;
    r = a * s.r;
    g = a * s.g;
    b = a * s.b;
}

float
ColorRgb::sumAbsComponents() const {
    return java::Math::abs(r) + java::Math::abs(g) + java::Math::abs(b);
}

void
ColorRgb::maximum(const ColorRgb s, const ColorRgb t) {
    r = s.r > t.r ? s.r : t.r;
    g = s.g > t.g ? s.g : t.g;
    b = s.b > t.b ? s.b : t.b;
}

void
ColorRgb::minimum(const ColorRgb s, const ColorRgb t) {
    r = s.r < t.r ? s.r : t.r;
    g = s.g < t.g ? s.g : t.g;
    b = s.b < t.b ? s.b : t.b;
}

float
ColorRgb::average() const {
    return (r + g + b) / 3.0F;
}

void
ColorRgb::interpolateBarycentric(const ColorRgb c0, const ColorRgb c1, const ColorRgb c2, const float u, const float v) {
    r = c0.r + u * (c1.r - c0.r) + v * (c2.r - c0.r);
    g = c0.g + u * (c1.g - c0.g) + v * (c2.g - c0.g);
    b = c0.b + u * (c1.b - c0.b) + v * (c2.b - c0.b);
}

void
ColorRgb::interpolateBiLinear(const ColorRgb c0, const ColorRgb c1, const ColorRgb c2, const ColorRgb c3, const float u, const float v) {
    const float c = u * v;
    const float bb = u - c;
    const float d = v - c;

    r = c0.r + b * (c1.r - c0.r) + c * (c2.r - c0.r) + d * (c3.r - c0.r);
    g = c0.g + b * (c1.g - c0.g) + c * (c2.g - c0.g) + d * (c3.g - c0.g);
    b = c0.b + bb * (c1.b - c0.b) + c * (c2.b - c0.b) + d * (c3.b - c0.b);
}

void
ColorRgb::clip() {
    if ( r < 0.0F ) {
        r = 0.0F;
    } else {
        r = r > 1.0F ? 1.0F : r;
    }

    if ( g < 0.0F ) {
        g = 0.0F;
    } else {
        g = g > 1.0F ? 1.0F : g;
    }

    if ( b < 0.0F ) {
        b = 0.0F;
    } else {
        b = b > 1.0F ? 1.0F : b;
    }
}

void
ColorRgb::print(java::PrintStream *stream) const {
    if ( stream == nullptr ) {
        return;
    }
    stream->printf("%g %g %g", r, g, b);
}
