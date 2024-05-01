#include "common/linealAlgebra/Float.h"
#include "common/ColorRgb.h"

ColorRgb::ColorRgb(const float inR, const float inG, const float inB) {
    r = inR;
    g = inG;
    b = inB;
}

void
ColorRgb::abs() {
    r = std::fabs(r);
    g = std::fabs(g);
    b = std::fabs(b);
}

void
colorsArrayCopy(ColorRgb *result, const ColorRgb *source, const char n) {
    for ( int i = 0; i < n; i++ ) {
        result[i] = source[i];
    }
}

void
colorsArrayAdd(ColorRgb *result, const ColorRgb *source, const char n) {
    for ( int i = 0; i < n; i++ ) {
        result[i].add(result[i], source[i]);
    }
}

void
colorsArrayClear(ColorRgb *color, const char n) {
    for ( int i = 0; i < n; i++ ) {
        color[i].clear();
    }
}

bool
ColorRgb::isBlack() const {
    return (r > -EPSILON && r < EPSILON &&
            g > -EPSILON && g < EPSILON &&
            b > -EPSILON && b < EPSILON);
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
    float a = (scale != 0.0f) ? 1.0f / scale : 1.0f;
    r = a * s.r;
    g = a * s.g;
    b = a * s.b;
}

float
ColorRgb::sumAbsComponents() const {
    return std::fabs(r) + std::fabs(g) + std::fabs(b);
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
    return (r + g + b) / 3.0f;
}

float
ColorRgb::gray() const {
    return spectrumGray(r, g, b);
}

float
ColorRgb::luminance() const {
    return spectrumLuminance(r, g, b);
}

void
ColorRgb::interpolateBarycentric(const ColorRgb c0, const ColorRgb c1, const ColorRgb c2, const float u, const float v) {
    r = c0.r + u * (c1.r - c0.r) + v * (c2.r - c0.r);
    g = c0.g + u * (c1.g - c0.g) + v * (c2.g - c0.g);
    b = c0.b + u * (c1.b - c0.b) + v * (c2.b - c0.b);
}

void
ColorRgb::interpolateBiLinear(const ColorRgb c0, const ColorRgb c1, const ColorRgb c2, const ColorRgb c3, const float u, const float v) {
    float c = u * v;
    float bb = u - c;
    float d = v - c;

    r = c0.r + b * (c1.r - c0.r) + c * (c2.r - c0.r) + d * (c3.r - c0.r);
    g = c0.g + b * (c1.g - c0.g) + c * (c2.g - c0.g) + d * (c3.g - c0.g);
    b = c0.b + bb * (c1.b - c0.b) + c * (c2.b - c0.b) + d * (c3.b - c0.b);
}

void
ColorRgb::clip() {
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
ColorRgb::print(FILE *fp) const {
    fprintf(fp, "%g %g %g", r, g, b);
}
