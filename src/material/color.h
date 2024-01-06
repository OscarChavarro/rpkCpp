#ifndef __COLOR__
#define __COLOR__

#include "material/spectrum.h"
#include "material/cie.h"
#include "material/rgb.h"

class COLOR {
  public:
    SPECTRUM spec;
    COLOR() : spec() {}

    inline void
    print(FILE *fp) {
        PrintSpectrum(fp, spec);
    }
};

inline void
colorClear(COLOR &c) {
    ClearSpectrum(c.spec);
}

inline void
colorSet(COLOR &c, float v1, float v2, float v3) {
    SetSpectrum((c).spec, v1, v2, v3);
}

inline void
colorSetMonochrome(COLOR &c, float v) {
    SetSpectrumMonochrome(c.spec, v);
}

inline bool
colorNull(COLOR &c) {
    return IsBlackSpectrum(c.spec);
}

inline void
colorScale(float a, COLOR &s, COLOR &r) {
    ScaleSpectrum(a, s.spec, r.spec);
}

inline void
colorProduct(COLOR &s, COLOR &t, COLOR &r) {
    MultiplySpectrum(s.spec, t.spec, r.spec);
}

inline void
colorProductScaled(COLOR &s, float a, COLOR &t, COLOR &r) {
    MultiplyScaledSpectrum(s.spec, a, t.spec, r.spec);
}

inline void
colorAdd(COLOR &s, COLOR &t, COLOR &r) {
    AddSpectrum(s.spec, t.spec, r.spec);
}

inline void
colorAddScaled(COLOR &s, float a, COLOR &t, COLOR &r) {
    AddScaledSpectrum((s).spec, (a), (t).spec, (r).spec);
}

inline void
colorAddConstant(COLOR &s, float a, COLOR &r) {
    AddConstantSpectrum(s.spec, a, r.spec);
}

inline void
colorSubtract(COLOR &s, COLOR & t, COLOR &r) {
    SubtractSpectrum(s.spec, t.spec, r.spec);
}

inline void
colorDivide(COLOR &s, COLOR &t, COLOR &r) {
    DivideSpectrum(s.spec, t.spec, r.spec);
}

inline void
colorScaleInverse(float a, COLOR &s, COLOR &r) {
    InverseScaleSpectrum(a, s.spec, r.spec);
}

#define COLORMAXCOMPONENT(s)        MaxSpectrumComponent((s).spec)
#define COLORSUMABSCOMPONENTS(s)    SumAbsSpectrumComponents((s).spec)
#define COLORABS(s, r)            AbsSpectrum((s).spec, (r).spec)
#define COLORMAX(s, t, r)        MaxSpectrum((s).spec, (t).spec, (r).spec)
#define COLORMIN(s, t, r)        MinSpectrum((s).spec, (t).spec, (r).spec)
#define COLORCLIPPOSITIVE(s, r)        ClipSpectrumPositive((s).spec, (r).spec)
#define COLORAVERAGE(s)            SpectrumAverage((s).spec)

#define ColorGray(s)            SpectrumGray((s).spec)
#define ColorLuminance(s)        SpectrumLuminance((s).spec)

#define COLORINTERPOLATEBARYCENTRIC(c0, c1, c2, u, v, c) \
  SpectrumInterpolateBarycentric(c0.spec, c1.spec, c2.spec, u, v, (c).spec)
#define COLORINTERPOLATEBILINEAR(c0, c1, c2, c3, u, v, c) \
  SpectrumInterpolateBilinear(c0.spec, c1.spec, c2.spec, c3.spec, u, v, (c).spec)

inline void
printCoefficients(FILE *fileDescriptor, COLOR *c, char n) {
    c[0].print(fileDescriptor);
    for ( int i = 1; i < n; i++ ) {
        fprintf(fileDescriptor, ", ");
        (c)[i].print(fileDescriptor);
    }
}

extern RGB *convertColorToRGB(COLOR col, RGB *rgb);
extern COLOR *convertRGBToColor(RGB rgb, COLOR *col);

#endif
