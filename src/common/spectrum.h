#ifndef __SPECTRUM__
#define __SPECTRUM__

#include "common/linealAlgebra/Float.h"

// Representation of radiance, radiosity, power, spectra
typedef float SPECTRUM[3];

inline void
scaleSpectrum(float a, const SPECTRUM spec, SPECTRUM &result) {
    const float *s = spec;
    float *r = result;

    *r++ = a * *s++;
    *r++ = a * *s++;
    *r = a * *s;
}

inline void
multiplySpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *s1 = spec1;
    const float *s2 = spec2;
    float *r = result;
    *r++ = *s1++ * *s2++;
    *r++ = *s1++ * *s2++;
    *r = *s1 * *s2;
}

inline void
multiplyScaledSpectrum(SPECTRUM &spec1, float a, SPECTRUM &spec2, SPECTRUM result) {
    const float *s1 = spec1;
    const float *s2 = spec2;
    float *r = result;
    *r++ = *s1++ * a * *s2++;
    *r++ = *s1++ * a * *s2++;
    *r = *s1 * a * *s2;
}

inline void
addSpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *s1 = spec1;
    const float *s2 = spec2;
    float *r = result;
    *r++ = *s1++ + *s2++;
    *r++ = *s1++ + *s2++;
    *r = *s1 + *s2;
}

inline void
addScaledSpectrum(SPECTRUM &spec1, float a, SPECTRUM &spec2, SPECTRUM &result) {
    const float *s1 = spec1;
    const float *s2 = spec2;
    float *r = result;
    *r++ = *s1++ + a * *s2++;
    *r++ = *s1++ + a * *s2++;
    *r = *s1 + a * *s2;
}

inline void
addConstantSpectrum(SPECTRUM &spec, float val, SPECTRUM &result) {
    const float *s = spec;
    float *r = result;
    *r++ = *s++ + val;
    *r++ = *s++ + val;
    *r = *s + val;
}

inline void
subtractSpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *s1 = spec1;
    const float *s2 = spec2;
    float *r = result;
    *r++ = *s1++ - *s2++;
    *r++ = *s1++ - *s2++;
    *r = *s1 - *s2;
}

inline void
divideSpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *s1 = spec1;
    const float *s2 = spec2;
    float *r = result;
    *r++ = (*s2 != 0.0) ? *s1 / *s2 : *s1; s1++; s2++;
    *r++ = (*s2 != 0.0) ? *s1 / *s2 : *s1; s1++; s2++;
    *r   = (*s2 != 0.0) ? *s1 / *s2 : *s1;
}

inline void
inverseScaleSpectrum(float scale, SPECTRUM &spec, SPECTRUM &result) {
    float a = (scale != 0.0f) ? 1.0f / scale : 1.0f;
    const float *s = spec;
    float *r = result;
    *r++ = a * *s++;
    *r++ = a * *s++;
    *r = a * *s;
}

inline float
maxSpectrumComponent(SPECTRUM &s) {
    return (s[0] > s[1] ? (s[0] > s[2] ? s[0] : s[2]) : (s[1] > s[2] ? s[1] : s[2]));
}

inline float
sumAbsSpectrumComponents(SPECTRUM &s) {
    return std::fabs(s[0]) + std::fabs(s[1]) + std::fabs(s[2]);
}

inline void
absSpectrum(SPECTRUM &spec, SPECTRUM &result) {
    const float *s = spec;
    float *r = result;
    *r++ = std::fabs(*s++);
    *r++ = std::fabs(*s++);
    *r = std::fabs(*s);
}

inline void
maxSpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *s = spec1;
    const float *t = spec2;
    float *r = result;
    *r++ = *s > *t ? *s : *t; s++; t++;
    *r++ = *s > *t ? *s : *t; s++; t++;
    *r = *s > *t ? *s : *t;
}

inline void
minSpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *s = spec1;
    const float *t = spec2;
    float *r = result;
    *r++ = *s < *t ? *s : *t; s++; t++;
    *r++ = *s < *t ? *s : *t; s++; t++;
    *r = *s < *t ? *s : *t;
}

inline void
spectrumInterpolateBarycentric(SPECTRUM &c0, SPECTRUM &c1, SPECTRUM &c2, float u, float v, SPECTRUM &c) {
    float _u = (u);
    float _v = (v);
    c[0] = c0[0] + _u * (c1[0] - c0[0]) + _v * (c2[0] - c0[0]);
    c[1] = c0[1] + _u * (c1[1] - c0[1]) + _v * (c2[1] - c0[1]);
    c[2] = c0[2] + _u * (c1[2] - c0[2]) + _v * (c2[2] - c0[2]);
}

inline void
spectrumInterpolateBiLinear(SPECTRUM &c0, SPECTRUM &c1, SPECTRUM &c2, SPECTRUM &c3, float u, float v, SPECTRUM &c) {
    float _c = u * v;
    float _b = u - _c;
    float _d = v - _c;
    c[0] = c0[0] + _b * (c1[0] - c0[0]) + _c * (c2[0] - c0[0])+ _d * (c3[0] - c0[0]);
    c[1] = c0[1] + _b * (c1[1] - c0[1]) + _c * (c2[1] - c0[1])+ _d * (c3[1] - c0[1]);
    c[2] = c0[2] + _b * (c1[2] - c0[2]) + _c * (c2[2] - c0[2])+ _d * (c3[2] - c0[2]);
}

#endif
