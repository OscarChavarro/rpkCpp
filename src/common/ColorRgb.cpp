#include "common/ColorRgb.h"

RGB *
convertColorToRGB(ColorRgb col, RGB *rgb) {
    setRGB(*rgb, col.spectrum[0], col.spectrum[1], col.spectrum[2]);
    return rgb;
}

ColorRgb *
convertRGBToColor(RGB rgb, ColorRgb *col) {
    col->set(rgb.r, rgb.g, rgb.b);
    return col;
}

void
ColorRgb::clear() {
    spectrum[0] = 0;
    spectrum[1] = 0;
    spectrum[2] = 0;
}

void
ColorRgb::set(float v1, float v2, float v3) {
    spectrum[0] = v1;
    spectrum[1] = v2;
    spectrum[2] = v3;
}

void
ColorRgb::setMonochrome(float v) {
    spectrum[0] = v;
    spectrum[1] = v;
    spectrum[2] = v;
}

bool
ColorRgb::isBlack() const {
    return (spectrum[0] > -EPSILON && spectrum[0] < EPSILON &&
            spectrum[1] > -EPSILON && spectrum[1] < EPSILON &&
            spectrum[2] > -EPSILON && spectrum[2] < EPSILON);
}

void
ColorRgb::scaledCopy(float a, const ColorRgb &c) {
    spectrum[0] = a * c.spectrum[0];
    spectrum[1] = a * c.spectrum[1];
    spectrum[2] = a * c.spectrum[2];
}

void
ColorRgb::scale(float a) {
    spectrum[0] *= a;
    spectrum[1] *= a;
    spectrum[2] *= a;
}

void
ColorRgb::scalarProduct(const ColorRgb &s, const ColorRgb &t) {
    spectrum[0] = s.spectrum[0] * t.spectrum[0];
    spectrum[1] = s.spectrum[1] * t.spectrum[1];
    spectrum[2] = s.spectrum[2] * t.spectrum[2];
}

void
ColorRgb::selfScalarProduct(const ColorRgb &s) {
    spectrum[0] *= s.spectrum[0];
    spectrum[1] *= s.spectrum[1];
    spectrum[2] *= s.spectrum[2];
}

void
ColorRgb::scalarProductScaled(ColorRgb &s, float a, ColorRgb &t) {
    spectrum[0] = s.spectrum[0] * a * t.spectrum[0];
    spectrum[1] = s.spectrum[1] * a * t.spectrum[1];
    spectrum[2] = s.spectrum[2] * a * t.spectrum[2];
}

void
ColorRgb::add(ColorRgb &s, ColorRgb &t) {
    spectrum[0] = s.spectrum[0] + t.spectrum[0];
    spectrum[1] = s.spectrum[1] + t.spectrum[1];
    spectrum[2] = s.spectrum[2] + t.spectrum[2];
}

void
ColorRgb::addScaled(ColorRgb &s, float a, ColorRgb &t) {
    spectrum[0] = s.spectrum[0] + a * t.spectrum[0];
    spectrum[1] = s.spectrum[1] + a * t.spectrum[1];
    spectrum[2] = s.spectrum[2] + a * t.spectrum[2];
}

void
ColorRgb::addConstant(ColorRgb &s, float a) {
    spectrum[0] = s.spectrum[0] + a;
    spectrum[1] = s.spectrum[1] + a;
    spectrum[2] = s.spectrum[2] + a;
}

void
ColorRgb::subtract(ColorRgb &s, ColorRgb & t) {
    spectrum[0] = s.spectrum[0] - t.spectrum[0];
    spectrum[1] = s.spectrum[1] - t.spectrum[1];
    spectrum[2] = s.spectrum[2] - t.spectrum[2];
}

void
ColorRgb::divide(ColorRgb &s, ColorRgb &t) {
    spectrum[0] = (t.spectrum[0] != 0.0) ? s.spectrum[0] / t.spectrum[0] : s.spectrum[0];
    spectrum[1] = (t.spectrum[1] != 0.0) ? s.spectrum[1] / t.spectrum[1] : s.spectrum[1];
    spectrum[2] = (t.spectrum[2] != 0.0) ? s.spectrum[2] / t.spectrum[2] : s.spectrum[2];
}

void
ColorRgb::scaleInverse(float scale, ColorRgb &s) {
    float a = (scale != 0.0f) ? 1.0f / scale : 1.0f;
    spectrum[0] = a * s.spectrum[0];
    spectrum[1] = a * s.spectrum[1];
    spectrum[2] = a * s.spectrum[2];
}

float
ColorRgb::maximumComponent() const {
    return (spectrum[0] > spectrum[1] ? (spectrum[0] > spectrum[2] ? spectrum[0] : spectrum[2]) : (spectrum[1] > spectrum[2] ? spectrum[1] : spectrum[2]));
}

float
ColorRgb::sumAbsComponents() const {
    return std::fabs(spectrum[0]) + std::fabs(spectrum[1]) + std::fabs(spectrum[2]);
}

void
ColorRgb::abs() {
    spectrum[0] = std::fabs(spectrum[0]);
    spectrum[1] = std::fabs(spectrum[1]);
    spectrum[2] = std::fabs(spectrum[2]);
}

void
ColorRgb::maximum(ColorRgb &s, ColorRgb &t) {
    spectrum[0] = s.spectrum[0] > t.spectrum[0] ? s.spectrum[0] : t.spectrum[0];
    spectrum[1] = s.spectrum[1] > t.spectrum[1] ? s.spectrum[1] : t.spectrum[1];
    spectrum[2] = s.spectrum[2] > t.spectrum[2] ? s.spectrum[2] : t.spectrum[2];
}

void
ColorRgb::minimum(ColorRgb &s, ColorRgb &t) {
    spectrum[0] = s.spectrum[0] < t.spectrum[0] ? s.spectrum[0] : t.spectrum[0];
    spectrum[1] = s.spectrum[1] < t.spectrum[1] ? s.spectrum[1] : t.spectrum[1];
    spectrum[2] = s.spectrum[2] < t.spectrum[2] ? s.spectrum[2] : t.spectrum[2];
}

float
ColorRgb::average() const {
    return (spectrum[0] + spectrum[1] + spectrum[2]) / 3.0f;
}
