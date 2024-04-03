#ifndef __SPECTRUM__
#define __SPECTRUM__

#include "common/linealAlgebra/Float.h"

// Representation of radiance, radiosity, power, spectra
typedef float SPECTRUM[3];

inline void
multiplySpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *s1 = spec1;
    const float *s2 = spec2;
    float *r = result;
    r[0] = s1[0] * s2[0];
    r[1] = s1[1] * s2[1];
    r[2] = s1[2] * s2[2];
}

inline void
multiplyScaledSpectrum(SPECTRUM &spec1, float a, SPECTRUM &spec2, SPECTRUM result) {
    const float *s1 = spec1;
    const float *s2 = spec2;
    float *r = result;
    r[0] = s1[0] * a * s2[0];
    r[1] = s1[1] * a * s2[1];
    r[2] = s1[2] * a * s2[2];
}

inline void
addSpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *s1 = spec1;
    const float *s2 = spec2;
    float *r = result;
    r[0] = s1[0] + s2[0];
    r[1] = s1[1] + s2[1];
    r[2] = s1[2] + s2[2];
}

inline void
addScaledSpectrum(SPECTRUM &spec1, float a, SPECTRUM &spec2, SPECTRUM &result) {
    const float *s1 = spec1;
    const float *s2 = spec2;
    float *r = result;
    r[0] = s1[0] + a * s2[0];
    r[1] = s1[1] + a * s2[1];
    r[2] = s1[2] + a * s2[2];
}

inline void
addConstantSpectrum(SPECTRUM &spec, float val, SPECTRUM &result) {
    const float *s = spec;
    float *r = result;
    r[0] = s[0] + val;
    r[1] = s[1] + val;
    r[2] = s[2] + val;
}

inline void
subtractSpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *s1 = spec1;
    const float *s2 = spec2;
    float *r = result;
    r[0] = s1[0] - s2[0];
    r[1] = s1[1] - s2[1];
    r[2] = s1[2] - s2[2];
}

inline void
divideSpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *s1 = spec1;
    const float *s2 = spec2;
    float *r = result;
    r[0] = (s2[0] != 0.0) ? s1[0] / s2[0] : s1[0];
    r[1] = (s2[1] != 0.0) ? s1[1] / s2[1] : s1[1];
    r[2] = (s2[2] != 0.0) ? s1[2] / s2[2] : s1[2];
}

inline void
inverseScaleSpectrum(float scale, SPECTRUM &spec, SPECTRUM &result) {
    float a = (scale != 0.0f) ? 1.0f / scale : 1.0f;
    const float *s = spec;
    float *r = result;
    r[0] = a * s[0];
    r[1] = a * s[1];
    r[2] = a * s[2];
}


inline void
absSpectrum(SPECTRUM &spec, SPECTRUM &result) {
    const float *s = spec;
    float *r = result;
    r[0] = std::fabs(s[0]);
    r[1] = std::fabs(s[1]);
    r[2] = std::fabs(s[2]);
}

inline void
maxSpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *s = spec1;
    const float *t = spec2;
    float *r = result;
    r[0] = s[0] > t[0] ? s[0] : t[0];
    r[1] = s[1] > t[1] ? s[1] : t[1];
    r[2] = s[2] > t[2] ? s[2] : t[2];
}

inline void
minSpectrum(SPECTRUM &spec1, SPECTRUM &spec2, SPECTRUM &result) {
    const float *s = spec1;
    const float *t = spec2;
    float *r = result;
    r[0] = s[0] < t[0] ? s[0] : t[0];
    r[1] = s[1] < t[1] ? s[1] : t[1];
    r[2] = s[2] < t[2] ? s[2] : t[2];
}

inline void
spectrumInterpolateBarycentric(SPECTRUM &c0, SPECTRUM &c1, SPECTRUM &c2, float u, float v, SPECTRUM &c) {
    c[0] = c0[0] + u * (c1[0] - c0[0]) + v * (c2[0] - c0[0]);
    c[1] = c0[1] + u * (c1[1] - c0[1]) + v * (c2[1] - c0[1]);
    c[2] = c0[2] + u * (c1[2] - c0[2]) + v * (c2[2] - c0[2]);
}

inline void
spectrumInterpolateBiLinear(SPECTRUM &c0, SPECTRUM &c1, SPECTRUM &c2, SPECTRUM &c3, float u, float v, SPECTRUM &spec) {
    float c = u * v;
    float b = u - c;
    float d = v - c;

    spec[0] = c0[0] + b * (c1[0] - c0[0]) + c * (c2[0] - c0[0]) + d * (c3[0] - c0[0]);
    spec[1] = c0[1] + b * (c1[1] - c0[1]) + c * (c2[1] - c0[1]) + d * (c3[1] - c0[1]);
    spec[2] = c0[2] + b * (c1[2] - c0[2]) + c * (c2[2] - c0[2]) + d * (c3[2] - c0[2]);
}

#endif
