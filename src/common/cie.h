/**
Converts mgf color specification into our representation of colors.
XYZ<->LUV conversions
*/

#ifndef __CIE__
#define __CIE__

/**
`MAX_EFFICACY'
`WHITE_EFFICACY'

Luminous efficacies [lm/W] for conversion between radiometric and
photometric units (in our case, for conversion between radiance and
luminance). Normally, the spectral value of a radiometric quantity would
be scaled by the photopic luminous efficiency function and integrated
over the visible spectrum; the result multiplied by MAX_EFFICACY would
give the appropriate photometric value. Without knowing the spectral
representation, it is generally impossible to perform correct
radometric->photometric conversion. The "WHITE_EFFICACY" factor is the
ratio between the luminour powers of the uniform equal-energy white
spectrum of 1W and its radiant power (which is, surprisingly, 1W). The
corrected CIE 1988 standard observer curve has been used in this case -
using the older CIE 1931 curves gives the value of 179 (see the Radiance
rendering system).
*/
// Uniform white light
#define WHITE_EFFICACY 183.07

extern void transformColorFromXYZ2RGB(float *xyz, float *rgb);
extern int clipGamut(float *rgb);

extern void computeColorConversionTransforms(
    float xr, float yr,
    float xg, float yg,
    float xb, float yb,
    float xw, float yw);

extern float getLuminousEfficacy();
extern float spectrumGray(const float r, const float g, const float b);
extern float spectrumLuminance(const float r, const float g, const float b);

#endif
