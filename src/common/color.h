#ifndef __COLOR__
#define __COLOR__

#include "common/cie.h"
#include "common/rgb.h"

/**
Representation of radiance, radiosity, power, spectra
*/
class COLOR {
  public:
    float spec[3];
    COLOR() : spec() {}

    inline void
    print(FILE *fp) {
        fprintf(fp, "%g %g %g", spec[0], spec[1], spec[2]);
    }
};

inline void
colorClear(COLOR &c) {
    c.spec[0] = 0;
    c.spec[1] = 0;
    c.spec[2] = 0;
}

inline void
colorSet(COLOR &c, float v1, float v2, float v3) {
    c.spec[0] = v1;
    c.spec[1] = v2;
    c.spec[2] = v3;
}

inline void
colorSetMonochrome(COLOR &c, float v) {
    c.spec[0] = v;
    c.spec[1] = v;
    c.spec[2] = v;
}

inline bool
colorNull(COLOR &c) {
    return (c.spec[0] > -EPSILON && c.spec[0] < EPSILON &&
            c.spec[1] > -EPSILON && c.spec[1] < EPSILON &&
            c.spec[2] > -EPSILON && c.spec[2] < EPSILON);
}

inline void
colorScale(float a, COLOR &c, COLOR &result) {
    result.spec[0] = a * c.spec[0];
    result.spec[1] = a * c.spec[1];
    result.spec[2] = a * c.spec[2];
}

inline void
colorProduct(COLOR &s, COLOR &t, COLOR &r) {
    r.spec[0] = s.spec[0] * t.spec[0];
    r.spec[1] = s.spec[1] * t.spec[1];
    r.spec[2] = s.spec[2] * t.spec[2];
}

inline void
colorProductScaled(COLOR &s, float a, COLOR &t, COLOR &r) {
    r.spec[0] = s.spec[0] * a * t.spec[0];
    r.spec[1] = s.spec[1] * a * t.spec[1];
    r.spec[2] = s.spec[2] * a * t.spec[2];
}

inline void
colorAdd(COLOR &s, COLOR &t, COLOR &r) {
    r.spec[0] = s.spec[0] + t.spec[0];
    r.spec[1] = s.spec[1] + t.spec[1];
    r.spec[2] = s.spec[2] + t.spec[2];
}

inline void
colorAddScaled(COLOR &s, float a, COLOR &t, COLOR &r) {
    r.spec[0] = s.spec[0] + a * t.spec[0];
    r.spec[1] = s.spec[1] + a * t.spec[1];
    r.spec[2] = s.spec[2] + a * t.spec[2];
}

inline void
colorAddConstant(COLOR &s, float a, COLOR &r) {
    r.spec[0] = s.spec[0] + a;
    r.spec[1] = s.spec[1] + a;
    r.spec[2] = s.spec[2] + a;
}

inline void
colorSubtract(COLOR &s, COLOR & t, COLOR &r) {
    r.spec[0] = s.spec[0] - t.spec[0];
    r.spec[1] = s.spec[1] - t.spec[1];
    r.spec[2] = s.spec[2] - t.spec[2];
}

inline void
colorDivide(COLOR &s, COLOR &t, COLOR &r) {
    r.spec[0] = (t.spec[0] != 0.0) ? s.spec[0] / t.spec[0] : s.spec[0];
    r.spec[1] = (t.spec[1] != 0.0) ? s.spec[1] / t.spec[1] : s.spec[1];
    r.spec[2] = (t.spec[2] != 0.0) ? s.spec[2] / t.spec[2] : s.spec[2];
}

inline void
colorScaleInverse(float scale, COLOR &s, COLOR &r) {
    float a = (scale != 0.0f) ? 1.0f / scale : 1.0f;
    r.spec[0] = a * s.spec[0];
    r.spec[1] = a * s.spec[1];
    r.spec[2] = a * s.spec[2];
}

inline float
colorMaximumComponent(COLOR &s) {
    return (s.spec[0] > s.spec[1] ? (s.spec[0] > s.spec[2] ? s.spec[0] : s.spec[2]) : (s.spec[1] > s.spec[2] ? s.spec[1] : s.spec[2]));
}

inline float
colorSumAbsComponents(COLOR &s) {
    return std::fabs(s.spec[0]) + std::fabs(s.spec[1]) + std::fabs(s.spec[2]);
}

inline void
colorAbs(COLOR &s, COLOR &r) {
    r.spec[0] = std::fabs(s.spec[0]);
    r.spec[1] = std::fabs(s.spec[1]);
    r.spec[2] = std::fabs(s.spec[2]);
}

inline void
colorMaximum(COLOR &s, COLOR &t, COLOR &r) {
    r.spec[0] = s.spec[0] > t.spec[0] ? s.spec[0] : t.spec[0];
    r.spec[1] = s.spec[1] > t.spec[1] ? s.spec[1] : t.spec[1];
    r.spec[2] = s.spec[2] > t.spec[2] ? s.spec[2] : t.spec[2];
}

inline void
colorMinimum(COLOR &s, COLOR &t, COLOR &r) {
    r.spec[0] = s.spec[0] < t.spec[0] ? s.spec[0] : t.spec[0];
    r.spec[1] = s.spec[1] < t.spec[1] ? s.spec[1] : t.spec[1];
    r.spec[2] = s.spec[2] < t.spec[2] ? s.spec[2] : t.spec[2];
}

inline float
colorAverage(COLOR &s) {
    return (s.spec[0] + s.spec[1] + s.spec[2]) / 3.0f;
}

inline float
colorGray(COLOR &s) {
    return spectrumGray(s.spec);
}

inline float
colorLuminance(COLOR &s) {
    return spectrumLuminance(s.spec);
}

inline void
colorInterpolateBarycentric(COLOR &c0, COLOR &c1, COLOR &c2, float u, float v, COLOR &r) {
    r.spec[0] = c0.spec[0] + u * (c1.spec[0] - c0.spec[0]) + v * (c2.spec[0] - c0.spec[0]);
    r.spec[1] = c0.spec[1] + u * (c1.spec[1] - c0.spec[1]) + v * (c2.spec[1] - c0.spec[1]);
    r.spec[2] = c0.spec[2] + u * (c1.spec[2] - c0.spec[2]) + v * (c2.spec[2] - c0.spec[2]);
}

inline void
colorInterpolateBiLinear(COLOR &c0, COLOR &c1, COLOR &c2, COLOR &c3, float u, float v, COLOR &r) {
    float c = u * v;
    float b = u - c;
    float d = v - c;

    r.spec[0] = c0.spec[0] + b * (c1.spec[0] - c0.spec[0]) + c * (c2.spec[0] - c0.spec[0]) + d * (c3.spec[0] - c0.spec[0]);
    r.spec[1] = c0.spec[1] + b * (c1.spec[1] - c0.spec[1]) + c * (c2.spec[1] - c0.spec[1]) + d * (c3.spec[1] - c0.spec[1]);
    r.spec[2] = c0.spec[2] + b * (c1.spec[2] - c0.spec[2]) + c * (c2.spec[2] - c0.spec[2]) + d * (c3.spec[2] - c0.spec[2]);
}

extern RGB *convertColorToRGB(COLOR col, RGB *rgb);
extern COLOR *convertRGBToColor(RGB rgb, COLOR *col);

#endif
