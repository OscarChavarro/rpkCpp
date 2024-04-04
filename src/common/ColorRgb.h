#ifndef __COLOR__
#define __COLOR__

#include "common/cie.h"
#include "common/rgb.h"

/**
Representation of radiance, radiosity, power, spectra
*/
class ColorRgb {
  public:
    float spectrum[3];
    ColorRgb() : spectrum() {}

    inline void
    print(FILE *fp) {
        fprintf(fp, "%g %g %g", spectrum[0], spectrum[1], spectrum[2]);
    }

    void clear();
};

inline void
colorSet(ColorRgb &c, float v1, float v2, float v3) {
    c.spectrum[0] = v1;
    c.spectrum[1] = v2;
    c.spectrum[2] = v3;
}

inline void
colorSetMonochrome(ColorRgb &c, float v) {
    c.spectrum[0] = v;
    c.spectrum[1] = v;
    c.spectrum[2] = v;
}

inline bool
colorNull(ColorRgb &c) {
    return (c.spectrum[0] > -EPSILON && c.spectrum[0] < EPSILON &&
            c.spectrum[1] > -EPSILON && c.spectrum[1] < EPSILON &&
            c.spectrum[2] > -EPSILON && c.spectrum[2] < EPSILON);
}

inline void
colorScale(float a, ColorRgb &c, ColorRgb &result) {
    result.spectrum[0] = a * c.spectrum[0];
    result.spectrum[1] = a * c.spectrum[1];
    result.spectrum[2] = a * c.spectrum[2];
}

inline void
colorProduct(ColorRgb &s, ColorRgb &t, ColorRgb &r) {
    r.spectrum[0] = s.spectrum[0] * t.spectrum[0];
    r.spectrum[1] = s.spectrum[1] * t.spectrum[1];
    r.spectrum[2] = s.spectrum[2] * t.spectrum[2];
}

inline void
colorProductScaled(ColorRgb &s, float a, ColorRgb &t, ColorRgb &r) {
    r.spectrum[0] = s.spectrum[0] * a * t.spectrum[0];
    r.spectrum[1] = s.spectrum[1] * a * t.spectrum[1];
    r.spectrum[2] = s.spectrum[2] * a * t.spectrum[2];
}

inline void
colorAdd(ColorRgb &s, ColorRgb &t, ColorRgb &r) {
    r.spectrum[0] = s.spectrum[0] + t.spectrum[0];
    r.spectrum[1] = s.spectrum[1] + t.spectrum[1];
    r.spectrum[2] = s.spectrum[2] + t.spectrum[2];
}

inline void
colorAddScaled(ColorRgb &s, float a, ColorRgb &t, ColorRgb &r) {
    r.spectrum[0] = s.spectrum[0] + a * t.spectrum[0];
    r.spectrum[1] = s.spectrum[1] + a * t.spectrum[1];
    r.spectrum[2] = s.spectrum[2] + a * t.spectrum[2];
}

inline void
colorAddConstant(ColorRgb &s, float a, ColorRgb &r) {
    r.spectrum[0] = s.spectrum[0] + a;
    r.spectrum[1] = s.spectrum[1] + a;
    r.spectrum[2] = s.spectrum[2] + a;
}

inline void
colorSubtract(ColorRgb &s, ColorRgb & t, ColorRgb &r) {
    r.spectrum[0] = s.spectrum[0] - t.spectrum[0];
    r.spectrum[1] = s.spectrum[1] - t.spectrum[1];
    r.spectrum[2] = s.spectrum[2] - t.spectrum[2];
}

inline void
colorDivide(ColorRgb &s, ColorRgb &t, ColorRgb &r) {
    r.spectrum[0] = (t.spectrum[0] != 0.0) ? s.spectrum[0] / t.spectrum[0] : s.spectrum[0];
    r.spectrum[1] = (t.spectrum[1] != 0.0) ? s.spectrum[1] / t.spectrum[1] : s.spectrum[1];
    r.spectrum[2] = (t.spectrum[2] != 0.0) ? s.spectrum[2] / t.spectrum[2] : s.spectrum[2];
}

inline void
colorScaleInverse(float scale, ColorRgb &s, ColorRgb &r) {
    float a = (scale != 0.0f) ? 1.0f / scale : 1.0f;
    r.spectrum[0] = a * s.spectrum[0];
    r.spectrum[1] = a * s.spectrum[1];
    r.spectrum[2] = a * s.spectrum[2];
}

inline float
colorMaximumComponent(ColorRgb &s) {
    return (s.spectrum[0] > s.spectrum[1] ? (s.spectrum[0] > s.spectrum[2] ? s.spectrum[0] : s.spectrum[2]) : (s.spectrum[1] > s.spectrum[2] ? s.spectrum[1] : s.spectrum[2]));
}

inline float
colorSumAbsComponents(ColorRgb &s) {
    return std::fabs(s.spectrum[0]) + std::fabs(s.spectrum[1]) + std::fabs(s.spectrum[2]);
}

inline void
colorAbs(ColorRgb &s, ColorRgb &r) {
    r.spectrum[0] = std::fabs(s.spectrum[0]);
    r.spectrum[1] = std::fabs(s.spectrum[1]);
    r.spectrum[2] = std::fabs(s.spectrum[2]);
}

inline void
colorMaximum(ColorRgb &s, ColorRgb &t, ColorRgb &r) {
    r.spectrum[0] = s.spectrum[0] > t.spectrum[0] ? s.spectrum[0] : t.spectrum[0];
    r.spectrum[1] = s.spectrum[1] > t.spectrum[1] ? s.spectrum[1] : t.spectrum[1];
    r.spectrum[2] = s.spectrum[2] > t.spectrum[2] ? s.spectrum[2] : t.spectrum[2];
}

inline void
colorMinimum(ColorRgb &s, ColorRgb &t, ColorRgb &r) {
    r.spectrum[0] = s.spectrum[0] < t.spectrum[0] ? s.spectrum[0] : t.spectrum[0];
    r.spectrum[1] = s.spectrum[1] < t.spectrum[1] ? s.spectrum[1] : t.spectrum[1];
    r.spectrum[2] = s.spectrum[2] < t.spectrum[2] ? s.spectrum[2] : t.spectrum[2];
}

inline float
colorAverage(ColorRgb &s) {
    return (s.spectrum[0] + s.spectrum[1] + s.spectrum[2]) / 3.0f;
}

inline float
colorGray(ColorRgb &s) {
    return spectrumGray(s.spectrum[0], s.spectrum[1], s.spectrum[2]);
}

inline float
colorLuminance(ColorRgb &s) {
    return spectrumLuminance(s.spectrum[0], s.spectrum[1], s.spectrum[2]);
}

inline void
colorInterpolateBarycentric(ColorRgb &c0, ColorRgb &c1, ColorRgb &c2, float u, float v, ColorRgb &r) {
    r.spectrum[0] = c0.spectrum[0] + u * (c1.spectrum[0] - c0.spectrum[0]) + v * (c2.spectrum[0] - c0.spectrum[0]);
    r.spectrum[1] = c0.spectrum[1] + u * (c1.spectrum[1] - c0.spectrum[1]) + v * (c2.spectrum[1] - c0.spectrum[1]);
    r.spectrum[2] = c0.spectrum[2] + u * (c1.spectrum[2] - c0.spectrum[2]) + v * (c2.spectrum[2] - c0.spectrum[2]);
}

inline void
colorInterpolateBiLinear(ColorRgb &c0, ColorRgb &c1, ColorRgb &c2, ColorRgb &c3, float u, float v, ColorRgb &r) {
    float c = u * v;
    float b = u - c;
    float d = v - c;

    r.spectrum[0] = c0.spectrum[0] + b * (c1.spectrum[0] - c0.spectrum[0]) + c * (c2.spectrum[0] - c0.spectrum[0]) + d * (c3.spectrum[0] - c0.spectrum[0]);
    r.spectrum[1] = c0.spectrum[1] + b * (c1.spectrum[1] - c0.spectrum[1]) + c * (c2.spectrum[1] - c0.spectrum[1]) + d * (c3.spectrum[1] - c0.spectrum[1]);
    r.spectrum[2] = c0.spectrum[2] + b * (c1.spectrum[2] - c0.spectrum[2]) + c * (c2.spectrum[2] - c0.spectrum[2]) + d * (c3.spectrum[2] - c0.spectrum[2]);
}

extern RGB *convertColorToRGB(ColorRgb col, RGB *rgb);
extern ColorRgb *convertRGBToColor(RGB rgb, ColorRgb *col);

#endif
