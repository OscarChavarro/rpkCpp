#ifndef __COLOR__
#define __COLOR__

#include "common/cie.h"
#include "common/rgb.h"

/**
Representation of radiance, radiosity, power, spectra
*/
class COLOR {
  public:
    float spectrum[3];
    COLOR() : spectrum() {}

    inline void
    print(FILE *fp) {
        fprintf(fp, "%g %g %g", spectrum[0], spectrum[1], spectrum[2]);
    }
};

inline void
colorClear(COLOR &c) {
    c.spectrum[0] = 0;
    c.spectrum[1] = 0;
    c.spectrum[2] = 0;
}

inline void
colorSet(COLOR &c, float v1, float v2, float v3) {
    c.spectrum[0] = v1;
    c.spectrum[1] = v2;
    c.spectrum[2] = v3;
}

inline void
colorSetMonochrome(COLOR &c, float v) {
    c.spectrum[0] = v;
    c.spectrum[1] = v;
    c.spectrum[2] = v;
}

inline bool
colorNull(COLOR &c) {
    return (c.spectrum[0] > -EPSILON && c.spectrum[0] < EPSILON &&
            c.spectrum[1] > -EPSILON && c.spectrum[1] < EPSILON &&
            c.spectrum[2] > -EPSILON && c.spectrum[2] < EPSILON);
}

inline void
colorScale(float a, COLOR &c, COLOR &result) {
    result.spectrum[0] = a * c.spectrum[0];
    result.spectrum[1] = a * c.spectrum[1];
    result.spectrum[2] = a * c.spectrum[2];
}

inline void
colorProduct(COLOR &s, COLOR &t, COLOR &r) {
    r.spectrum[0] = s.spectrum[0] * t.spectrum[0];
    r.spectrum[1] = s.spectrum[1] * t.spectrum[1];
    r.spectrum[2] = s.spectrum[2] * t.spectrum[2];
}

inline void
colorProductScaled(COLOR &s, float a, COLOR &t, COLOR &r) {
    r.spectrum[0] = s.spectrum[0] * a * t.spectrum[0];
    r.spectrum[1] = s.spectrum[1] * a * t.spectrum[1];
    r.spectrum[2] = s.spectrum[2] * a * t.spectrum[2];
}

inline void
colorAdd(COLOR &s, COLOR &t, COLOR &r) {
    r.spectrum[0] = s.spectrum[0] + t.spectrum[0];
    r.spectrum[1] = s.spectrum[1] + t.spectrum[1];
    r.spectrum[2] = s.spectrum[2] + t.spectrum[2];
}

inline void
colorAddScaled(COLOR &s, float a, COLOR &t, COLOR &r) {
    r.spectrum[0] = s.spectrum[0] + a * t.spectrum[0];
    r.spectrum[1] = s.spectrum[1] + a * t.spectrum[1];
    r.spectrum[2] = s.spectrum[2] + a * t.spectrum[2];
}

inline void
colorAddConstant(COLOR &s, float a, COLOR &r) {
    r.spectrum[0] = s.spectrum[0] + a;
    r.spectrum[1] = s.spectrum[1] + a;
    r.spectrum[2] = s.spectrum[2] + a;
}

inline void
colorSubtract(COLOR &s, COLOR & t, COLOR &r) {
    r.spectrum[0] = s.spectrum[0] - t.spectrum[0];
    r.spectrum[1] = s.spectrum[1] - t.spectrum[1];
    r.spectrum[2] = s.spectrum[2] - t.spectrum[2];
}

inline void
colorDivide(COLOR &s, COLOR &t, COLOR &r) {
    r.spectrum[0] = (t.spectrum[0] != 0.0) ? s.spectrum[0] / t.spectrum[0] : s.spectrum[0];
    r.spectrum[1] = (t.spectrum[1] != 0.0) ? s.spectrum[1] / t.spectrum[1] : s.spectrum[1];
    r.spectrum[2] = (t.spectrum[2] != 0.0) ? s.spectrum[2] / t.spectrum[2] : s.spectrum[2];
}

inline void
colorScaleInverse(float scale, COLOR &s, COLOR &r) {
    float a = (scale != 0.0f) ? 1.0f / scale : 1.0f;
    r.spectrum[0] = a * s.spectrum[0];
    r.spectrum[1] = a * s.spectrum[1];
    r.spectrum[2] = a * s.spectrum[2];
}

inline float
colorMaximumComponent(COLOR &s) {
    return (s.spectrum[0] > s.spectrum[1] ? (s.spectrum[0] > s.spectrum[2] ? s.spectrum[0] : s.spectrum[2]) : (s.spectrum[1] > s.spectrum[2] ? s.spectrum[1] : s.spectrum[2]));
}

inline float
colorSumAbsComponents(COLOR &s) {
    return std::fabs(s.spectrum[0]) + std::fabs(s.spectrum[1]) + std::fabs(s.spectrum[2]);
}

inline void
colorAbs(COLOR &s, COLOR &r) {
    r.spectrum[0] = std::fabs(s.spectrum[0]);
    r.spectrum[1] = std::fabs(s.spectrum[1]);
    r.spectrum[2] = std::fabs(s.spectrum[2]);
}

inline void
colorMaximum(COLOR &s, COLOR &t, COLOR &r) {
    r.spectrum[0] = s.spectrum[0] > t.spectrum[0] ? s.spectrum[0] : t.spectrum[0];
    r.spectrum[1] = s.spectrum[1] > t.spectrum[1] ? s.spectrum[1] : t.spectrum[1];
    r.spectrum[2] = s.spectrum[2] > t.spectrum[2] ? s.spectrum[2] : t.spectrum[2];
}

inline void
colorMinimum(COLOR &s, COLOR &t, COLOR &r) {
    r.spectrum[0] = s.spectrum[0] < t.spectrum[0] ? s.spectrum[0] : t.spectrum[0];
    r.spectrum[1] = s.spectrum[1] < t.spectrum[1] ? s.spectrum[1] : t.spectrum[1];
    r.spectrum[2] = s.spectrum[2] < t.spectrum[2] ? s.spectrum[2] : t.spectrum[2];
}

inline float
colorAverage(COLOR &s) {
    return (s.spectrum[0] + s.spectrum[1] + s.spectrum[2]) / 3.0f;
}

inline float
colorGray(COLOR &s) {
    return spectrumGray(s.spectrum);
}

inline float
colorLuminance(COLOR &s) {
    return spectrumLuminance(s.spectrum);
}

inline void
colorInterpolateBarycentric(COLOR &c0, COLOR &c1, COLOR &c2, float u, float v, COLOR &r) {
    r.spectrum[0] = c0.spectrum[0] + u * (c1.spectrum[0] - c0.spectrum[0]) + v * (c2.spectrum[0] - c0.spectrum[0]);
    r.spectrum[1] = c0.spectrum[1] + u * (c1.spectrum[1] - c0.spectrum[1]) + v * (c2.spectrum[1] - c0.spectrum[1]);
    r.spectrum[2] = c0.spectrum[2] + u * (c1.spectrum[2] - c0.spectrum[2]) + v * (c2.spectrum[2] - c0.spectrum[2]);
}

inline void
colorInterpolateBiLinear(COLOR &c0, COLOR &c1, COLOR &c2, COLOR &c3, float u, float v, COLOR &r) {
    float c = u * v;
    float b = u - c;
    float d = v - c;

    r.spectrum[0] = c0.spectrum[0] + b * (c1.spectrum[0] - c0.spectrum[0]) + c * (c2.spectrum[0] - c0.spectrum[0]) + d * (c3.spectrum[0] - c0.spectrum[0]);
    r.spectrum[1] = c0.spectrum[1] + b * (c1.spectrum[1] - c0.spectrum[1]) + c * (c2.spectrum[1] - c0.spectrum[1]) + d * (c3.spectrum[1] - c0.spectrum[1]);
    r.spectrum[2] = c0.spectrum[2] + b * (c1.spectrum[2] - c0.spectrum[2]) + c * (c2.spectrum[2] - c0.spectrum[2]) + d * (c3.spectrum[2] - c0.spectrum[2]);
}

extern RGB *convertColorToRGB(COLOR col, RGB *rgb);
extern COLOR *convertRGBToColor(RGB rgb, COLOR *col);

#endif
