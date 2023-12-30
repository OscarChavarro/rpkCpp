/* spectrum.h: representation of radiance, radiosity, power, ...
 * spectra */

#ifndef _RPK_SPECTRUM_H_
#define _RPK_SPECTRUM_H_

#include <cstdio>

#include "common/linealAlgebra/Float.h"
#include "material/spectrum_type.h"

#define InitChannels2(spec1, chan1, spec2, chan2) const float *chan1 = (spec1); float *chan2 = (spec2);


#define InitChannels3(spec1, chan1, spec2, chan2, spec3, chan3) const float *chan1 = (spec1), *chan2 = (spec2); float *chan3 = (spec3);

#define PrintSpectrum(fp, s)                \
{                            \
  fprintf(fp, "%g %g %g", s[0], s[1], s[2]);        \
}

#define ClearSpectrum(/*SPECTRUM*/ s)            \
{                            \
  s[0] = s[1] = s[2] = 0;                \
}

#define SetSpectrum(/*SPECTRUM*/ s, /*float*/ c1, /*float*/ c2, /*float*/ c3) \
{                            \
  s[0] = c1; s[1] = c2; s[2] = c3;            \
}

#define SetSpectrumMonochrome(/*SPECTRUM*/ s, /*float*/ val)    \
{                            \
  s[0] = s[1] = s[2] = val;                \
}

#define CopySpectrum(/*SPECTRUM*/ dest, /*SPECTRUM*/ src)    \
{                            \
  InitChannels2(src, _s, dest, _r);            \
  *_r++ = *_s++;                        \
  *_r++ = *_s++;                        \
  *_r   = *_s  ;                        \
}

#define IsBlackSpectrum(/*SPECTRUM*/ s)            \
  (s[0] > -EPSILON && s[0] < EPSILON &&            \
   s[1] > -EPSILON && s[1] < EPSILON &&            \
   s[2] > -EPSILON && s[2] < EPSILON)

#define ScaleSpectrum(/*float*/ a, /*SPECTRUM*/ spec, /*SPECTRUM*/ result) \
{                            \
  InitChannels2(spec, _s, result, _r);            \
  *_r++ = (a) * *_s++;                    \
  *_r++ = (a) * *_s++;                    \
  *_r   = (a) * *_s;                    \
}

#define MultSpectrum(/*SPECTRUM*/ spec1, /*SPECTRUM*/ spec2, /*SPECTRUM*/ result) \
{                            \
  InitChannels3(spec1, _s1, spec2, _s2, result, _r);    \
  *_r++ = *_s1++ * *_s2++;                    \
  *_r++ = *_s1++ * *_s2++;                    \
  *_r   = *_s1   * *_s2  ;                    \
}

#define MultScaledSpectrum(/*SPECTRUM*/ spec1, /*float*/ a, /*SPECTRUM*/ spec2, /*SPECTRUM*/ result) \
{                            \
  InitChannels3(spec1, _s1, spec2, _s2, result, _r);    \
  *_r++ = *_s1++ * (a) * *_s2++;                \
  *_r++ = *_s1++ * (a) * *_s2++;                \
  *_r   = *_s1   * (a) * *_s2  ;                \
}

#define ScalarSpectrumProduct(/*SPECTRUM*/ s, /*SPECTRUM*/ t)    \
  (s[0] * t[0] + s[1] * t[1] + s[2] * t[2])

#define AddSpectrum(/*SPECTRUM*/ spec1, /*SPECTRUM*/ spec2, /*SPECTRUM*/ result) \
{                            \
  InitChannels3(spec1, _s1, spec2, _s2, result, _r);    \
  *_r++ = *_s1++ + *_s2++;                    \
  *_r++ = *_s1++ + *_s2++;                    \
  *_r   = *_s1   + *_s2  ;                    \
}

#define AddScaledSpectrum(/*SPECTRUM*/ spec1, /*float*/ a, /*SPECTRUM*/ spec2, /*SPECTRUM*/ result) \
{                            \
  InitChannels3(spec1, _s1, spec2, _s2, result, _r);    \
  *_r++ = *_s1++ + (a) * *_s2++;                \
  *_r++ = *_s1++ + (a) * *_s2++;                \
  *_r   = *_s1   + (a) * *_s2  ;                \
}

#define AddConstantSpectrum(/*SPECTRUM*/ spec, /*float*/ val, /*SPECTRUM*/ result) \
{                            \
  InitChannels2(spec, _s, result, _r);            \
  *_r++ = *_s++ + (val);                    \
  *_r++ = *_s++ + (val);                    \
  *_r   = *_s   + (val);                    \
}

#define SubtractSpectrum(/*SPECTRUM*/ spec1, /*SPECTRUM*/ spec2, /*SPECTRUM*/ result) \
{                            \
  InitChannels3(spec1, _s1, spec2, _s2, result, _r);    \
  *_r++ = *_s1++ - *_s2++;                    \
  *_r++ = *_s1++ - *_s2++;                    \
  *_r   = *_s1   - *_s2  ;                    \
}

#define SubtractScaledSpectrum(/*SPECTRUM*/ spec1, /*float*/ a, /*SPECTRUM*/ spec2, /*SPECTRUM*/ result) \
{                            \
  InitChannels3(spec1, _s1, spec2, _s2, result, _r);    \
  *_r++ = *_s1++ - (a) * *_s2++;                \
  *_r++ = *_s1++ - (a) * *_s2++;                \
  *_r   = *_s1   - (a) * *_s2  ;                \
}

#define DivideSpectrum(/*SPECTRUM*/ spec1, /*SPECTRUM*/ spec2, /*SPECTRUM*/ result) \
{                            \
  InitChannels3(spec1, _s1, spec2, _s2, result, _r);    \
  *_r++ = (*_s2 != 0.) ? *_s1 / *_s2 : *_s1; _s1++; _s2++;    \
  *_r++ = (*_s2 != 0.) ? *_s1 / *_s2 : *_s1; _s1++; _s2++;    \
  *_r   = (*_s2 != 0.) ? *_s1 / *_s2 : *_s1;            \
}

#define InverseScaleSpectrum(/*float*/ a, /*SPECTRUM*/ spec, /*SPECTRUM*/ result) \
{                            \
  float _a = ((a) != 0.) ? 1./(a) : 1.;            \
  InitChannels2(spec, _s, result, _r);            \
  *_r++ = _a * *_s++;                    \
  *_r++ = _a * *_s++;                    \
  *_r   = _a * *_s  ;                    \
}

#define MaxSpectrumComponent(/*SPECTRUM*/ s)        \
  (s[0] > s[1] ? (s[0] > s[2] ? s[0] : s[2]) : (s[1] > s[2] ? s[1] : s[2]))

#define MinSpectrumComponent(/*SPECTRUM*/ s)        \
  (s[0] < s[1] ? (s[0] < s[2] ? s[0] : s[2]) : (s[1] < s[2] ? s[1] : s[2]))

#define SumAbsSpectrumComponents(/*SPECTRUM*/ s)        \
  (fabs(s[0]) + fabs(s[1]) + fabs(s[2]))

#define AbsSpectrum(/*SPECTRUM*/ spec, /*SPECTRUM*/ result)    \
{                            \
  InitChannels2(spec, _s, result, _r);            \
  *_r++ = fabs(*_s++);                    \
  *_r++ = fabs(*_s++);                    \
  *_r   = fabs(*_s  );                    \
}

#define MaxSpectrum(/*SPECTRUM*/ spec1, /*SPECTRUM*/ spec2, /*SPECTRUM*/ result) \
{                            \
  InitChannels3(spec1, _s, spec2, _t, result, _r);        \
  *_r++ = *_s > *_t ? *_s : *_t; _s++; _t++;            \
  *_r++ = *_s > *_t ? *_s : *_t; _s++; _t++;            \
  *_r   = *_s > *_t ? *_s : *_t;                \
}

#define MinSpectrum(/*SPECTRUM*/ spec1, /*SPECTRUM*/ spec2, /*SPECTRUM*/ result) \
{                            \
  InitChannels3(spec1, _s, spec2, _t, result, _r);        \
  *_r++ = *_s < *_t ? *_s : *_t; _s++; _t++;            \
  *_r++ = *_s < *_t ? *_s : *_t; _s++; _t++;            \
  *_r   = *_s < *_t ? *_s : *_t;                \
}

#define ClipSpectrumPositive(/*SPECTRUM*/ spec, result) \
{\
  InitChannels2(spec, _s, result, _r);            \
  *_r++ = *_s > 0.0 ? *_s : 0.0; _s++;                    \
  *_r++ = *_s > 0.0 ? *_s : 0.0; _s++;                    \
  *_r   = *_s > 0.0 ? *_s : 0.0; _s++;                    \
}

#define SpectrumAverage(/*SPECTRUM*/ s)            \
  ((s[0] + s[1] + s[2]) / 3.0)

#define SpectrumInterpolateBarycentric(/*SPECTRUM*/ c0, /*SPECTRUM*/ c1, /*SPECTRUM*/ c2, /*float*/ u, /*float*/ v, /*SPECTRUM*/ c) \
{                            \
  double _u = (u), _v = (v);                \
  (c)[0] = (c0)[0] + _u * ((c1)[0] - (c0)[0]) + _v * ((c2)[0] - (c0)[0]); \
  (c)[1] = (c0)[1] + _u * ((c1)[1] - (c0)[1]) + _v * ((c2)[1] - (c0)[1]); \
  (c)[2] = (c0)[2] + _u * ((c1)[2] - (c0)[2]) + _v * ((c2)[2] - (c0)[2]); \
}

#define SpectrumInterpolateBilinear(/*SPECTRUM*/ c0, /*SPECTRUM*/ c1, /*SPECTRUM*/ c2, /*SPECTRUM*/ c3, /*float*/ u, /*float*/ v, /*SPECTRUM*/ c) \
{                            \
  double _c=(u)*(v), _b=(u)-_c, _d=(v)-_c;        \
  (c)[0] = (c0)[0] + (_b) * ((c1)[0] - (c0)[0]) + (_c) * ((c2)[0] - (c0)[0])+ (_d) * ((c3)[0] - (c0)[0]); \
  (c)[1] = (c0)[1] + (_b) * ((c1)[1] - (c0)[1]) + (_c) * ((c2)[1] - (c0)[1])+ (_d) * ((c3)[1] - (c0)[1]); \
  (c)[2] = (c0)[2] + (_b) * ((c1)[2] - (c0)[2]) + (_c) * ((c2)[2] - (c0)[2])+ (_d) * ((c3)[2] - (c0)[2]); \
}

#define SpectrumGammaCorrect(/*SPECTRUM*/ _c, /*float*/ _g) \
        { \
          float _ig = 1.0/_g; \
          _c[0] = pow(_c[0], _ig); \
          _c[1] = pow(_c[1], _ig); \
          _c[2] = pow(_c[2], _ig); \
        }

#endif
