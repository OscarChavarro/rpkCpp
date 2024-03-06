#ifndef __COLOR__
#define __COLOR__

#include "common/spectrum.h"
#include "common/cie.h"
#include "common/rgb.h"

class COLOR {
  public:
    SPECTRUM spec;
    COLOR() : spec() {}

    inline void
    print(FILE *fp) {
        printSpectrum(fp, spec);
    }
};

inline void
colorClear(COLOR &c) {
    clearSpectrum(c.spec);
}

inline void
colorSet(COLOR &c, float v1, float v2, float v3) {
    setSpectrum((c).spec, v1, v2, v3);
}

inline void
colorSetMonochrome(COLOR &c, float v) {
    setSpectrumMonochrome(c.spec, v);
}

inline bool
colorNull(COLOR &c) {
    return isBlackSpectrum(c.spec);
}

inline void
colorScale(float a, COLOR &s, COLOR &r) {
    scaleSpectrum(a, s.spec, r.spec);
}

inline void
colorProduct(COLOR &s, COLOR &t, COLOR &r) {
    multiplySpectrum(s.spec, t.spec, r.spec);
}

inline void
colorProductScaled(COLOR &s, float a, COLOR &t, COLOR &r) {
    multiplyScaledSpectrum(s.spec, a, t.spec, r.spec);
}

inline void
colorAdd(COLOR &s, COLOR &t, COLOR &r) {
    addSpectrum(s.spec, t.spec, r.spec);
}

inline void
colorAddScaled(COLOR &s, float a, COLOR &t, COLOR &r) {
    addScaledSpectrum((s).spec, (a), (t).spec, (r).spec);
}

inline void
colorAddConstant(COLOR &s, float a, COLOR &r) {
    addConstantSpectrum(s.spec, a, r.spec);
}

inline void
colorSubtract(COLOR &s, COLOR & t, COLOR &r) {
    subtractSpectrum(s.spec, t.spec, r.spec);
}

inline void
colorDivide(COLOR &s, COLOR &t, COLOR &r) {
    divideSpectrum(s.spec, t.spec, r.spec);
}

inline void
colorScaleInverse(float a, COLOR &s, COLOR &r) {
    inverseScaleSpectrum(a, s.spec, r.spec);
}

inline float
colorMaximumComponent(COLOR &s) {
    return maxSpectrumComponent(s.spec);
}

inline float
colorSumAbsComponents(COLOR &s) {
    return sumAbsSpectrumComponents(s.spec);
}

inline void
colorAbs(COLOR &s, COLOR &r) {
    absSpectrum(s.spec, r.spec);
}

inline void
colorMaximum(COLOR &s, COLOR &t, COLOR &r) {
    maxSpectrum(s.spec, t.spec, r.spec);
}

inline void
colorMinimum(COLOR &s, COLOR &t, COLOR &r) {
    minSpectrum(s.spec, t.spec, r.spec);
}

inline float
colorAverage(COLOR &s) {
    return spectrumAverage(s.spec);
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
colorInterpolateBarycentric(COLOR &c0, COLOR &c1, COLOR &c2, float u, float v, COLOR &c) {
    spectrumInterpolateBarycentric(c0.spec, c1.spec, c2.spec, u, v, c.spec);
}

inline void
colorInterpolateBiLinear(COLOR &c0, COLOR &c1, COLOR &c2, COLOR &c3, float u, float v, COLOR &c) {
    spectrumInterpolateBilinear(c0.spec, c1.spec, c2.spec, c3.spec, u, v, c.spec);
}

extern RGB *convertColorToRGB(COLOR col, RGB *rgb);
extern COLOR *convertRGBToColor(RGB rgb, COLOR *col);

#endif
