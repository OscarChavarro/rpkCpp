/**
Converts mgf color specification into our representation of colors.
XYZ<->LUV conversions
*/

#ifndef __CIE__
#define __CIE__

// Uniform white light
extern const float WHITE_EFFICACY;

extern void transformColorFromXYZ2RGB(const float *xyz, float *rgb);
extern int clipGamut(float *rgb);

extern void computeColorConversionTransforms(
    float xr, float yr,
    float xg, float yg,
    float xb, float yb,
    float xw, float yw);

extern float getLuminousEfficacy();
extern float spectrumGray(float r, float g, float b);
extern float spectrumLuminance(float r, float g, float b);

#endif
