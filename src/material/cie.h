/*
converts mgf color specification into our representation of colors.
*/

#ifndef _CIE_H_
#define _CIE_H_

/* jp: for XYZ<->LUV conversions */

/* ---------------------------------------------------------------------------
  `MAX_EFFICACY'
  `WHITE_EFFICACY'

  Luminous efficacies [lm/W] for conversion between radiometric and
  photometric units (in our case, for conversion between radiance and
  luminance). Normaly, the spectral value of a radiometric quantity would
  be scaled by the photopic luminous efficeincy function and integrated
  over the visible spectrum; the result multiplied by "MAX_EFFICACY would
  give the appropriate photometric value. Without knowing the spectral
  represnetation, it is generally impossible to perform correct
  radometric->photometric conversion. The "WHITE_EFFICACY" factor is the
  ratio between the luminour powes of the uniform equal-energy white
  spectrum of 1W and its radiant power (which is, surprisingly, 1W). The
  corrected CIE 1988 standard observer curve has been used in this case -
  using the older CIE 1931 curves gives the value of 179 (see the Radiance
  rendering system). 
  ------------------------------------------------------------------------- */
#define WHITE_EFFICACY 183.07 /* uniform white light */

extern void xyz_rgb(float *xyz, float *rgb);

/* returns TRUE if the color was desaturated during clipping against the
 * monitor gamut */
extern int clipgamut(float *rgb);

/* computes RGB <-> XYZ color transforms based on the 
 * given monitor primary colors and whitepoint */
extern void ComputeColorConversionTransforms(float xr, float yr,
                                             float xg, float yg,
                                             float xb, float yb,
                                             float xw, float yw);

/* ---------------------------------------------------------------------------
  `GetLuminousEfficacy'
  `SetLuminousEfficacy'

  Set/return the value usef for tristimulus white efficacy.
  ------------------------------------------------------------------------- */
extern void GetLuminousEfficacy(float *e);

extern float SpectrumGray(const float * spec);

extern float SpectrumLuminance(const float * spec);

#endif
