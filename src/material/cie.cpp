#include "material/cie.h"

/* This code is a modified version of the CIE XYZ<->RGB code
 * in the mgf documentation */

static float CIE_x_r = 0.640;    /* nominal CRT primaries */
static float CIE_y_r = 0.330;
static float CIE_x_g = 0.290;
static float CIE_y_g = 0.600;
static float CIE_x_b = 0.150;
static float CIE_y_b = 0.060;
static float CIE_x_w = 0.3333333333;
static float CIE_y_w = 0.3333333333;

#define CIE_D           (       CIE_x_r*(CIE_y_g - CIE_y_b) + \
                                CIE_x_g*(CIE_y_b - CIE_y_r) + \
                                CIE_x_b*(CIE_y_r - CIE_y_g)     )
#define CIE_C_rD        ( (1./CIE_y_w) * \
                                ( CIE_x_w*(CIE_y_g - CIE_y_b) - \
                                  CIE_y_w*(CIE_x_g - CIE_x_b) + \
                                  CIE_x_g*CIE_y_b - CIE_x_b*CIE_y_g     ) )
#define CIE_C_gD        ( (1./CIE_y_w) * \
                                ( CIE_x_w*(CIE_y_b - CIE_y_r) - \
                                  CIE_y_w*(CIE_x_b - CIE_x_r) - \
                                  CIE_x_r*CIE_y_b + CIE_x_b*CIE_y_r     ) )
#define CIE_C_bD        ( (1./CIE_y_w) * \
                                ( CIE_x_w*(CIE_y_r - CIE_y_g) - \
                                  CIE_y_w*(CIE_x_r - CIE_x_g) + \
                                  CIE_x_r*CIE_y_g - CIE_x_g*CIE_y_r     ) )

#define CIE_rf          (CIE_y_r*CIE_C_rD/CIE_D)
#define CIE_gf          (CIE_y_g*CIE_C_gD/CIE_D)
#define CIE_bf          (CIE_y_b*CIE_C_bD/CIE_D)

/* ---------------------------------------------------------------------------
  `globalLuminousEfficacy'

  Luminous efficacy currently in use.
  ------------------------------------------------------------------------- */
static float globalLuminousEfficacy = WHITE_EFFICACY;

/* ---------------------------------------------------------------------------
  `globalXyz2RgbMat'
  `globalRgb2XyzMat'

  Conversion matrices for CIEXYZ<->RGB conversions. The actual values are
  set up according to specified monitor primaries. 
  ------------------------------------------------------------------------- */
static float globalXyz2RgbMat[3][3];
static float globalRgb2XyzMat[3][3];

/* ---------------------------------------------------------------------------
                                                             PRIVATE FUNCTIONS
  ------------------------------------------------------------------------- */
static float
gray(const float *spec) {
    return CIE_rf * spec[0] + CIE_gf * spec[1] + CIE_bf * spec[2];
}

static float
luminance(const float *spec) {
    return globalLuminousEfficacy * gray(spec);
}

static void
setColorTransform(float mat[3][3],
                  float a, float b, float c,
                  float d, float e, float f,
                  float g, float h, float i) {
    mat[0][0] = a;
    mat[0][1] = b;
    mat[0][2] = c;
    mat[1][0] = d;
    mat[1][1] = e;
    mat[1][2] = f;
    mat[2][0] = g;
    mat[2][1] = h;
    mat[2][2] = i;
}

static void
colorTransform(float *col, float mat[3][3], float *res) {
    res[0] = mat[0][0] * col[0] + mat[0][1] * col[1] + mat[0][2] * col[2];
    res[1] = mat[1][0] * col[0] + mat[1][1] * col[1] + mat[1][2] * col[2];
    res[2] = mat[2][0] * col[0] + mat[2][1] * col[1] + mat[2][2] * col[2];
}

/* ---------------------------------------------------------------------------
                                                              PUBLIC FUNCTIONS
  ----------------------------------------------------------------------------
  `getLuminousEfficacy'
  `SetLuminousEfficacy'

  Set/return the value usef for tristimulus white efficacy.
  ------------------------------------------------------------------------- */
void
getLuminousEfficacy(float *e) {
    *e = globalLuminousEfficacy;
}

/* ---------------------------------------------------------------------------
  `spectrumGray'

  Returns an achromatic value representing the spectral quantity.
  ------------------------------------------------------------------------- */
float
spectrumGray(const float *spec) {
    return gray(spec);
}

/* ---------------------------------------------------------------------------
  `spectrumLuminance'

  Returns the luminance, photometric quantity correspoinding to the
  radiance of the given spectrum. 
  ------------------------------------------------------------------------- */
float
spectrumLuminance(const float *spec) {
    return luminance(spec);
}

/* ---------------------------------------------------------------------------
  `computeColorConversionTransforms'

  Computes RGB <-> XYZ color transforms based on the given monitor primary
  colors and whitepoint.
  ------------------------------------------------------------------------- */
void
computeColorConversionTransforms(float xr, float yr,
                                 float xg, float yg,
                                 float xb, float yb,
                                 float xw, float yw) {
    CIE_x_r = xr;
    CIE_y_r = yr;
    CIE_x_g = xg;
    CIE_y_g = yg;
    CIE_x_b = xb;
    CIE_y_b = yb;
    CIE_x_w = xw;
    CIE_y_w = yw;

    setColorTransform(globalXyz2RgbMat,    /* XYZ to RGB */
                      (CIE_y_g - CIE_y_b - CIE_x_b * CIE_y_g + CIE_y_b * CIE_x_g) / CIE_C_rD,
                      (CIE_x_b - CIE_x_g - CIE_x_b * CIE_y_g + CIE_x_g * CIE_y_b) / CIE_C_rD,
                      (CIE_x_g * CIE_y_b - CIE_x_b * CIE_y_g) / CIE_C_rD,
                      (CIE_y_b - CIE_y_r - CIE_y_b * CIE_x_r + CIE_y_r * CIE_x_b) / CIE_C_gD,
                      (CIE_x_r - CIE_x_b - CIE_x_r * CIE_y_b + CIE_x_b * CIE_y_r) / CIE_C_gD,
                      (CIE_x_b * CIE_y_r - CIE_x_r * CIE_y_b) / CIE_C_gD,
                      (CIE_y_r - CIE_y_g - CIE_y_r * CIE_x_g + CIE_y_g * CIE_x_r) / CIE_C_bD,
                      (CIE_x_g - CIE_x_r - CIE_x_g * CIE_y_r + CIE_x_r * CIE_y_g) / CIE_C_bD,
                      (CIE_x_r * CIE_y_g - CIE_x_g * CIE_y_r) / CIE_C_bD);

    setColorTransform(globalRgb2XyzMat,    /* RGB to XYZ */
                      CIE_x_r * CIE_C_rD / CIE_D, CIE_x_g * CIE_C_gD / CIE_D, CIE_x_b * CIE_C_bD / CIE_D,
                      CIE_y_r * CIE_C_rD / CIE_D, CIE_y_g * CIE_C_gD / CIE_D, CIE_y_b * CIE_C_bD / CIE_D,
                      (1. - CIE_x_r - CIE_y_r) * CIE_C_rD / CIE_D,
                      (1. - CIE_x_g - CIE_y_g) * CIE_C_gD / CIE_D,
                      (1. - CIE_x_b - CIE_y_b) * CIE_C_bD / CIE_D);
}

/* ---------------------------------------------------------------------------
                                                               CIE XYZ <-> RGB
  ------------------------------------------------------------------------- */
void
transformColorFromXYZ2RGB(float *xyz, float *rgb) {
    colorTransform(xyz, globalXyz2RgbMat, rgb);
}

/* ---------------------------------------------------------------------------
                                                                          MISC
  ------------------------------------------------------------------------- */

/* Returns TRUE if the color was desaturated during gamut clipping. */
int
clipGamut(float *rgb) {
    /* really SHOULD desaturate instead of just clipping! */
    int i, desaturated = 0;
    for ( i = 0; i < 3; i++ ) {
        if ( rgb[i] < 0. ) {
            rgb[i] = 0.;
            desaturated = 1;
        }
    }
    return desaturated;
}
