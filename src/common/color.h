#ifndef __COLOR__
#define __COLOR__

#include "common/cie.h"
#include "common/rgb.h"

/**
Representation of radiance, radiosity, power, spectra
*/
class COLOR {
  public:
    float r;
    float g;
    float b;
    COLOR() : r(), g(), b() {}

    inline void
    print(FILE *fp) {
        fprintf(fp, "%g %g %g", r, g, b);
    }
};

inline void
colorClear(COLOR &c) {
    c.r = 0;
    c.g = 0;
    c.b = 0;
}

inline void
colorSet(COLOR &c, float v1, float v2, float v3) {
    c.r = v1;
    c.g = v2;
    c.b = v3;
}

inline void
colorSetMonochrome(COLOR &c, float v) {
    c.r = v;
    c.g = v;
    c.b = v;
}

inline bool
colorNull(COLOR &c) {
    return (c.r > -EPSILON && c.r < EPSILON &&
            c.g > -EPSILON && c.g < EPSILON &&
            c.b > -EPSILON && c.b < EPSILON);
}

inline void
colorScale(float a, COLOR &c, COLOR &result) {
    result.r = a * c.r;
    result.g = a * c.g;
    result.b = a * c.b;
}

inline void
colorProduct(COLOR &s, COLOR &t, COLOR &r) {
    r.r = s.r * t.r;
    r.g = s.g * t.g;
    r.b = s.b * t.b;
}

inline void
colorProductScaled(COLOR &s, float a, COLOR &t, COLOR &r) {
    r.r = s.r * a * t.r;
    r.g = s.g * a * t.g;
    r.b = s.b * a * t.b;
}

inline void
colorAdd(COLOR &s, COLOR &t, COLOR &r) {
    r.r = s.r + t.r;
    r.g = s.g + t.g;
    r.b = s.b + t.b;
}

inline void
colorAddScaled(COLOR &s, float a, COLOR &t, COLOR &r) {
    r.r = s.r + a * t.r;
    r.g = s.g + a * t.g;
    r.b = s.b + a * t.b;
}

inline void
colorAddConstant(COLOR &s, float a, COLOR &r) {
    r.r = s.r + a;
    r.g = s.g + a;
    r.b = s.b + a;
}

inline void
colorSubtract(COLOR &s, COLOR & t, COLOR &r) {
    r.r = s.r - t.r;
    r.g = s.g - t.g;
    r.b = s.b - t.b;
}

inline void
colorDivide(COLOR &s, COLOR &t, COLOR &r) {
    r.r = (t.r != 0.0) ? s.r / t.r : s.r;
    r.g = (t.g != 0.0) ? s.g / t.g : s.g;
    r.b = (t.b != 0.0) ? s.b / t.b : s.b;
}

inline void
colorScaleInverse(float scale, COLOR &s, COLOR &r) {
    float a = (scale != 0.0f) ? 1.0f / scale : 1.0f;
    r.r = a * s.r;
    r.g = a * s.g;
    r.b = a * s.b;
}

inline float
colorMaximumComponent(COLOR &s) {
    return (s.r > s.g ? (s.r > s.b ? s.r : s.b) : (s.g > s.b ? s.g : s.b));
}

inline float
colorSumAbsComponents(COLOR &s) {
    return std::fabs(s.r) + std::fabs(s.g) + std::fabs(s.b);
}

inline void
colorAbs(COLOR &s, COLOR &r) {
    r.r = std::fabs(s.r);
    r.g = std::fabs(s.g);
    r.b = std::fabs(s.b);
}

inline void
colorMaximum(COLOR &s, COLOR &t, COLOR &r) {
    r.r = s.r > t.r ? s.r : t.r;
    r.g = s.g > t.g ? s.g : t.g;
    r.b = s.b > t.b ? s.b : t.b;
}

inline void
colorMinimum(COLOR &s, COLOR &t, COLOR &r) {
    r.r = s.r < t.r ? s.r : t.r;
    r.g = s.g < t.g ? s.g : t.g;
    r.b = s.b < t.b ? s.b : t.b;
}

inline float
colorAverage(COLOR &s) {
    return (s.r + s.g + s.b) / 3.0f;
}

inline float
colorGray(COLOR &s) {
    return spectrumGray(s.r, s.g, s.b);
}

inline float
colorLuminance(COLOR &s) {
    return spectrumLuminance(s.r, s.g, s.b);
}

inline void
colorInterpolateBarycentric(COLOR &c0, COLOR &c1, COLOR &c2, float u, float v, COLOR &r) {
    r.r = c0.r + u * (c1.r - c0.r) + v * (c2.r - c0.r);
    r.g = c0.g + u * (c1.g - c0.g) + v * (c2.g - c0.g);
    r.b = c0.b + u * (c1.b - c0.b) + v * (c2.b - c0.b);
}

inline void
colorInterpolateBiLinear(COLOR &c0, COLOR &c1, COLOR &c2, COLOR &c3, float u, float v, COLOR &r) {
    float c = u * v;
    float b = u - c;
    float d = v - c;

    r.r = c0.r + b * (c1.r - c0.r) + c * (c2.r - c0.r) + d * (c3.r - c0.r);
    r.g = c0.g + b * (c1.g - c0.g) + c * (c2.g - c0.g) + d * (c3.g - c0.g);
    r.b = c0.b + b * (c1.b - c0.b) + c * (c2.b - c0.b) + d * (c3.b - c0.b);
}

extern RGB *convertColorToRGB(COLOR col, RGB *rgb);
extern COLOR *convertRGBToColor(RGB rgb, COLOR *col);

#endif
