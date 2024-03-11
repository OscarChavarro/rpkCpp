#ifndef __SPECTRUM__
#define __SPECTRUM__

#include <cstdio>

#include "common/linealAlgebra/Float.h"

// Representation of radiance, radiosity, power, spectra
typedef float SPECTRUM[3];

inline void
printSpectrum(FILE *fp, SPECTRUM &s) {
    fprintf(fp, "%g %g %g", s[0], s[1], s[2]);
}

inline void
clearSpectrum(SPECTRUM &s) {
    s[0] = s[1] = s[2] = 0;
}

inline void
setSpectrum(SPECTRUM &s, float c1, float c2, float c3) {
    s[0] = c1; s[1] = c2; s[2] = c3;
}

inline void
setSpectrumMonochrome(SPECTRUM &s, float val) {
    s[0] = s[1] = s[2] = val;
}

inline bool
isBlackSpectrum(SPECTRUM &s) {
    return (s[0] > -EPSILON && s[0] < EPSILON &&
            s[1] > -EPSILON && s[1] < EPSILON &&
            s[2] > -EPSILON && s[2] < EPSILON);
}

inline void
scaleSpectrum(float a, const SPECTRUM spec, SPECTRUM &result) {
    const float *_s = spec;
    float *_r = result;

    *_r++ = a * *_s++;
    *_r++ = a * *_s++;
    *_r = a * *_s;
}

inline void
multiplySpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *_s1 = spec1;
    const float *_s2 = spec2;
    float *_r = result;
    *_r++ = *_s1++ * *_s2++;
    *_r++ = *_s1++ * *_s2++;
    *_r = *_s1 * *_s2  ;
}

inline void
multiplyScaledSpectrum(SPECTRUM &spec1, float a, SPECTRUM &spec2, SPECTRUM result) {
    const float *_s1 = spec1;
    const float *_s2 = spec2;
    float *_r = result;
    *_r++ = *_s1++ * a * *_s2++;
    *_r++ = *_s1++ * a * *_s2++;
    *_r = *_s1 * a * *_s2 ;
}

inline void
addSpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *_s1 = spec1;
    const float *_s2 = spec2;
    float *_r = result;
    *_r++ = *_s1++ + *_s2++;
    *_r++ = *_s1++ + *_s2++;
    *_r = *_s1 + *_s2;
}

inline void
addScaledSpectrum(SPECTRUM &spec1, float a, SPECTRUM &spec2, SPECTRUM &result) {
    const float *_s1 = spec1;
    const float *_s2 = spec2;
    float *_r = result;
    *_r++ = *_s1++ + a * *_s2++;
    *_r++ = *_s1++ + a * *_s2++;
    *_r = *_s1 + a * *_s2;
}

inline void
addConstantSpectrum(SPECTRUM &spec, float val, SPECTRUM &result) {
    const float *_s = spec;
    float *_r = result;
    *_r++ = *_s++ + val;
    *_r++ = *_s++ + val;
    *_r = *_s + val;
}

inline void
subtractSpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *_s1 = spec1;
    const float *_s2 = spec2;
    float *_r = result;
    *_r++ = *_s1++ - *_s2++;
    *_r++ = *_s1++ - *_s2++;
    *_r = *_s1 - *_s2;
}

inline void
divideSpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *_s1 = spec1;
    const float *_s2 = spec2;
    float *_r = result;
    *_r++ = (*_s2 != 0.0) ? *_s1 / *_s2 : *_s1; _s1++; _s2++;
    *_r++ = (*_s2 != 0.0) ? *_s1 / *_s2 : *_s1; _s1++; _s2++;
    *_r   = (*_s2 != 0.0) ? *_s1 / *_s2 : *_s1;
}

inline void
inverseScaleSpectrum(float a, SPECTRUM &spec, SPECTRUM &result) {
    float _a = (a != 0.0f) ? 1.0f/a : 1.0f;
    const float *_s = spec;
    float *_r = result;
    *_r++ = _a * *_s++;
    *_r++ = _a * *_s++;
    *_r   = _a * *_s  ;
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
    const float *_s = spec;
    float *_r = result;
    *_r++ = std::fabs(*_s++);
    *_r++ = std::fabs(*_s++);
    *_r = std::fabs(*_s);
}

inline void
maxSpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *_s = spec1;
    const float *_t = spec2;
    float *_r = result;
    *_r++ = *_s > *_t ? *_s : *_t; _s++; _t++;
    *_r++ = *_s > *_t ? *_s : *_t; _s++; _t++;
    *_r = *_s > *_t ? *_s : *_t;
}

inline void
minSpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *_s = spec1;
    const float *_t = spec2;
    float *_r = result;
    *_r++ = *_s < *_t ? *_s : *_t; _s++; _t++;
    *_r++ = *_s < *_t ? *_s : *_t; _s++; _t++;
    *_r = *_s < *_t ? *_s : *_t;
}

inline float
spectrumAverage(SPECTRUM &s) {
    return (s[0] + s[1] + s[2]) / 3.0f;
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
