#ifndef __COLOR__
#define __COLOR__

#include "common/cie.h"
#include "common/rgb.h"

/**
Representation of radiance, radiosity, power, spectra
*/
class ColorRgb {
  public:
    float r;
    float g;
    float b;
    ColorRgb() : r(), g(), b() {}

    inline void
    print(FILE *fp) {
        fprintf(fp, "%g %g %g", r, g, b);
    }
};

inline void
colorClear(ColorRgb &c) {
    c.r = 0;
    c.g = 0;
    c.b = 0;
}

inline void
colorSet(ColorRgb &c, float v1, float v2, float v3) {
    c.r = v1;
    c.g = v2;
    c.b = v3;
}

inline void
colorSetMonochrome(ColorRgb &c, float v) {
    c.r = v;
    c.g = v;
    c.b = v;
}

inline bool
colorNull(ColorRgb &c) {
    return (c.r > -EPSILON && c.r < EPSILON &&
            c.g > -EPSILON && c.g < EPSILON &&
            c.b > -EPSILON && c.b < EPSILON);
}

inline void
colorScale(float a, ColorRgb &c, ColorRgb &result) {
    result.r = a * c.r;
    result.g = a * c.g;
    result.b = a * c.b;
}

inline void
colorProduct(ColorRgb &s, ColorRgb &t, ColorRgb &r) {
    r.r = s.r * t.r;
    r.g = s.g * t.g;
    r.b = s.b * t.b;
}

inline void
colorProductScaled(ColorRgb &s, float a, ColorRgb &t, ColorRgb &r) {
    r.r = s.r * a * t.r;
    r.g = s.g * a * t.g;
    r.b = s.b * a * t.b;
}

inline void
colorAdd(ColorRgb &s, ColorRgb &t, ColorRgb &r) {
    r.r = s.r + t.r;
    r.g = s.g + t.g;
    r.b = s.b + t.b;
}

inline void
colorAddScaled(ColorRgb &s, float a, ColorRgb &t, ColorRgb &r) {
    r.r = s.r + a * t.r;
    r.g = s.g + a * t.g;
    r.b = s.b + a * t.b;
}

inline void
colorAddConstant(ColorRgb &s, float a, ColorRgb &r) {
    r.r = s.r + a;
    r.g = s.g + a;
    r.b = s.b + a;
}

inline void
colorSubtract(ColorRgb &s, ColorRgb & t, ColorRgb &r) {
    r.r = s.r - t.r;
    r.g = s.g - t.g;
    r.b = s.b - t.b;
}

inline void
colorDivide(ColorRgb &s, ColorRgb &t, ColorRgb &r) {
    r.r = (t.r != 0.0) ? s.r / t.r : s.r;
    r.g = (t.g != 0.0) ? s.g / t.g : s.g;
    r.b = (t.b != 0.0) ? s.b / t.b : s.b;
}

inline void
colorScaleInverse(float scale, ColorRgb &s, ColorRgb &r) {
    float a = (scale != 0.0f) ? 1.0f / scale : 1.0f;
    r.r = a * s.r;
    r.g = a * s.g;
    r.b = a * s.b;
}

inline float
colorMaximumComponent(ColorRgb &s) {
    return (s.r > s.g ? (s.r > s.b ? s.r : s.b) : (s.g > s.b ? s.g : s.b));
}

inline float
colorSumAbsComponents(ColorRgb &s) {
    return std::fabs(s.r) + std::fabs(s.g) + std::fabs(s.b);
}

inline void
colorAbs(ColorRgb &s, ColorRgb &r) {
    r.r = std::fabs(s.r);
    r.g = std::fabs(s.g);
    r.b = std::fabs(s.b);
}

inline void
colorMaximum(ColorRgb &s, ColorRgb &t, ColorRgb &r) {
    r.r = s.r > t.r ? s.r : t.r;
    r.g = s.g > t.g ? s.g : t.g;
    r.b = s.b > t.b ? s.b : t.b;
}

inline void
colorMinimum(ColorRgb &s, ColorRgb &t, ColorRgb &r) {
    r.r = s.r < t.r ? s.r : t.r;
    r.g = s.g < t.g ? s.g : t.g;
    r.b = s.b < t.b ? s.b : t.b;
}

inline float
colorAverage(ColorRgb &s) {
    return (s.r + s.g + s.b) / 3.0f;
}

inline float
colorGray(ColorRgb &s) {
    return spectrumGray(s.r, s.g, s.b);
}

inline float
colorLuminance(ColorRgb &s) {
    return spectrumLuminance(s.r, s.g, s.b);
}

inline void
colorInterpolateBarycentric(ColorRgb &c0, ColorRgb &c1, ColorRgb &c2, float u, float v, ColorRgb &r) {
    r.r = c0.r + u * (c1.r - c0.r) + v * (c2.r - c0.r);
    r.g = c0.g + u * (c1.g - c0.g) + v * (c2.g - c0.g);
    r.b = c0.b + u * (c1.b - c0.b) + v * (c2.b - c0.b);
}

inline void
colorInterpolateBiLinear(ColorRgb &c0, ColorRgb &c1, ColorRgb &c2, ColorRgb &c3, float u, float v, ColorRgb &r) {
    float c = u * v;
    float b = u - c;
    float d = v - c;

    r.r = c0.r + b * (c1.r - c0.r) + c * (c2.r - c0.r) + d * (c3.r - c0.r);
    r.g = c0.g + b * (c1.g - c0.g) + c * (c2.g - c0.g) + d * (c3.g - c0.g);
    r.b = c0.b + b * (c1.b - c0.b) + c * (c2.b - c0.b) + d * (c3.b - c0.b);
}

extern RGB *convertColorToRGB(ColorRgb col, RGB *rgb);
extern ColorRgb *convertRGBToColor(RGB rgb, ColorRgb *col);

#endif
