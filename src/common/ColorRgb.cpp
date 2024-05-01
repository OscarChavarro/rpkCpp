#include "common/mymath.h"
#include "common/linealAlgebra/Float.h"
#include "common/ColorRgb.h"

ColorRgb::ColorRgb():
    r(),
    g(),
    b()
{
}

ColorRgb::ColorRgb(float inR, float inG, float inB) {
    r = inR;
    g = inG;
    b = inB;
}

void
ColorRgb::clear() {
    r = 0;
    g = 0;
    b = 0;
}

void
ColorRgb::set(float v1, float v2, float v3) {
    r = v1;
    g = v2;
    b = v3;
}

void
ColorRgb::setMonochrome(float v) {
    r = v;
    g = v;
    b = v;
}

bool
ColorRgb::isBlack() const {
    return (r > -EPSILON && r < EPSILON &&
            g > -EPSILON && g < EPSILON &&
            b > -EPSILON && b < EPSILON);
}

void
ColorRgb::scaledCopy(float a, const ColorRgb c) {
    r = a * c.r;
    g = a * c.g;
    b = a * c.b;
}

void
ColorRgb::scale(float a) {
    r *= a;
    g *= a;
    b *= a;
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
ColorRgb::scalarProductScaled(ColorRgb s, float a, ColorRgb t) {
    r = s.r * a * t.r;
    g = s.g * a * t.g;
    b = s.b * a * t.b;
}

void
ColorRgb::add(ColorRgb s, ColorRgb t) {
    r = s.r + t.r;
    g = s.g + t.g;
    b = s.b + t.b;
}

void
ColorRgb::addScaled(ColorRgb s, float a, ColorRgb t) {
    r = s.r + a * t.r;
    g = s.g + a * t.g;
    b = s.b + a * t.b;
}

void
ColorRgb::addConstant(ColorRgb s, float a) {
    r = s.r + a;
    g = s.g + a;
    b = s.b + a;
}

void
ColorRgb::subtract(ColorRgb s, ColorRgb  t) {
    r = s.r - t.r;
    g = s.g - t.g;
    b = s.b - t.b;
}

void
ColorRgb::divide(ColorRgb s, ColorRgb t) {
    r = (t.r != 0.0) ? s.r / t.r : s.r;
    g = (t.g != 0.0) ? s.g / t.g : s.g;
    b = (t.b != 0.0) ? s.b / t.b : s.b;
}

void
ColorRgb::scaleInverse(float scale, ColorRgb s) {
    float a = (scale != 0.0f) ? 1.0f / scale : 1.0f;
    r = a * s.r;
    g = a * s.g;
    b = a * s.b;
}

float
ColorRgb::maximumComponent() const {
    if ( r > g ) {
        return r > b ? r : b;
    } else {
        return g > b ? g : b;
    }
}

float
ColorRgb::sumAbsComponents() const {
    return std::fabs(r) + std::fabs(g) + std::fabs(b);
}

void
ColorRgb::abs() {
    r = std::fabs(r);
    g = std::fabs(g);
    b = std::fabs(b);
}

void
ColorRgb::maximum(ColorRgb s, ColorRgb t) {
    r = s.r > t.r ? s.r : t.r;
    g = s.g > t.g ? s.g : t.g;
    b = s.b > t.b ? s.b : t.b;
}

void
ColorRgb::minimum(ColorRgb s, ColorRgb t) {
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
ColorRgb::interpolateBarycentric(ColorRgb c0, ColorRgb c1, ColorRgb c2, float u, float v) {
    r = c0.r + u * (c1.r - c0.r) + v * (c2.r - c0.r);
    g = c0.g + u * (c1.g - c0.g) + v * (c2.g - c0.g);
    b = c0.b + u * (c1.b - c0.b) + v * (c2.b - c0.b);
}

void
ColorRgb::interpolateBiLinear(ColorRgb c0, ColorRgb c1, ColorRgb c2, ColorRgb c3, float u, float v) {
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
