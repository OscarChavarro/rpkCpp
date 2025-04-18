#include "common/cie.h"

/**
This code is a modified version of the CIE XYZ<->RGB code
in the mgf documentation
*/

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
const float WHITE_EFFICACY = 183.07f;

static float CIE_x_r = 0.640f; // Nominal CRT primaries
static float CIE_y_r = 0.330f;
static float CIE_x_g = 0.290f;
static float CIE_y_g = 0.600f;
static float CIE_x_b = 0.150f;
static float CIE_y_b = 0.060f;
static float CIE_x_w = 0.3333333333f;
static float CIE_y_w = 0.3333333333f;

#define CIE_D ( \
    CIE_x_r * (CIE_y_g - CIE_y_b) + \
    CIE_x_g * (CIE_y_b - CIE_y_r) + \
    CIE_x_b * (CIE_y_r - CIE_y_g) \
)

#define CIE_C_rD ( \
    (1.0/CIE_y_w) * \
        ( \
            CIE_x_w*(CIE_y_g - CIE_y_b) - \
            CIE_y_w*(CIE_x_g - CIE_x_b) + \
            CIE_x_g*CIE_y_b - CIE_x_b*CIE_y_g \
        ) \
    )

#define CIE_C_gD ( \
    (1.0/CIE_y_w) * \
        ( \
            CIE_x_w*(CIE_y_b - CIE_y_r) - \
            CIE_y_w*(CIE_x_b - CIE_x_r) - \
            CIE_x_r*CIE_y_b + CIE_x_b*CIE_y_r \
        ) \
    )

#define CIE_C_bD ( \
    (1.0/CIE_y_w) * \
        ( \
            CIE_x_w*(CIE_y_r - CIE_y_g) - \
            CIE_y_w*(CIE_x_r - CIE_x_g) + \
            CIE_x_r*CIE_y_g - CIE_x_g*CIE_y_r \
        ) \
    )

#define CIE_rf (CIE_y_r * CIE_C_rD / CIE_D)
#define CIE_gf (CIE_y_g * CIE_C_gD / CIE_D)
#define CIE_bf (CIE_y_b * CIE_C_bD / CIE_D)

/**
Luminous efficacy currently in use.
*/
static float globalLuminousEfficacy = WHITE_EFFICACY;

/**
Conversion matrices for CIE XYZ<->RGB conversions. The actual values are
set up according to specified monitor primaries.
*/
static float globalXyz2RgbMat[3][3];
static float globalRgb2XyzMat[3][3];

static float
gray(const float r, const float g, const float b) {
    return (float)CIE_rf * r + (float)CIE_gf * g + (float)CIE_bf * b;
}

static float
luminance(const float r, const float g, const float b) {
    return globalLuminousEfficacy * gray(r, g, b);
}

static void
setColorTransform(
    float mat[3][3],
    float a, float b, float c,
    float d, float e, float f,
    float g, float h, float i)
{
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
colorTransform(const float *col, float mat[3][3], float *res) {
    res[0] = mat[0][0] * col[0] + mat[0][1] * col[1] + mat[0][2] * col[2];
    res[1] = mat[1][0] * col[0] + mat[1][1] * col[1] + mat[1][2] * col[2];
    res[2] = mat[2][0] * col[0] + mat[2][1] * col[1] + mat[2][2] * col[2];
}

/**
Set/return the value used for tri-stimulus white efficacy.
*/
float
getLuminousEfficacy() {
    return globalLuminousEfficacy;
}

/**
Returns an achromatic value representing the spectral quantity.
*/
float
spectrumGray(const float r, const float g, const float b) {
    return gray(r, g, b);
}

/**
Returns the luminance, photometric quantity corresponding to the
radiance of the given spectrum.
*/
float
spectrumLuminance(const float r, const float g, const float b) {
    return luminance(r, g, b);
}

/**
Computes RGB <-> XYZ color transforms based on the given monitor primary
colors and white point.
*/
void
computeColorConversionTransforms(
    float xr,
    float yr,
    float xg,
    float yg,
    float xb,
    float yb,
    float xw,
    float yw)
{
    CIE_x_r = xr;
    CIE_y_r = yr;
    CIE_x_g = xg;
    CIE_y_g = yg;
    CIE_x_b = xb;
    CIE_y_b = yb;
    CIE_x_w = xw;
    CIE_y_w = yw;

    setColorTransform(
            globalXyz2RgbMat, // XYZ to RGB
          (float)((CIE_y_g - CIE_y_b - CIE_x_b * CIE_y_g + CIE_y_b * CIE_x_g) / CIE_C_rD),
            (float)((CIE_x_b - CIE_x_g - CIE_x_b * CIE_y_g + CIE_x_g * CIE_y_b) / CIE_C_rD),
            (float)((CIE_x_g * CIE_y_b - CIE_x_b * CIE_y_g) / CIE_C_rD),
            (float)((CIE_y_b - CIE_y_r - CIE_y_b * CIE_x_r + CIE_y_r * CIE_x_b) / CIE_C_gD),
            (float)((CIE_x_r - CIE_x_b - CIE_x_r * CIE_y_b + CIE_x_b * CIE_y_r) / CIE_C_gD),
            (float)((CIE_x_b * CIE_y_r - CIE_x_r * CIE_y_b) / CIE_C_gD),
            (float)((CIE_y_r - CIE_y_g - CIE_y_r * CIE_x_g + CIE_y_g * CIE_x_r) / CIE_C_bD),
            (float)((CIE_x_g - CIE_x_r - CIE_x_g * CIE_y_r + CIE_x_r * CIE_y_g) / CIE_C_bD),
            (float)((CIE_x_r * CIE_y_g - CIE_x_g * CIE_y_r) / CIE_C_bD));

    setColorTransform(
            globalRgb2XyzMat, // RGB to XYZ
          (float)(CIE_x_r * CIE_C_rD / CIE_D),
            (float)(CIE_x_g * CIE_C_gD / CIE_D),
            (float)(CIE_x_b * CIE_C_bD / CIE_D),
            (float)(CIE_y_r * CIE_C_rD / CIE_D),
            (float)(CIE_y_g * CIE_C_gD / CIE_D),
            (float)(CIE_y_b * CIE_C_bD / CIE_D),
            (float)((1.0 - CIE_x_r - CIE_y_r) * CIE_C_rD / CIE_D),
            (float)((1.0 - CIE_x_g - CIE_y_g) * CIE_C_gD / CIE_D),
            (float)((1.0 - CIE_x_b - CIE_y_b) * CIE_C_bD / CIE_D));
}

/**
CIE XYZ <-> RGB
*/
void
transformColorFromXYZ2RGB(const float *xyz, float *rgb) {
    colorTransform(xyz, globalXyz2RgbMat, rgb);
}

/**
Returns TRUE if the color was desaturated during clipping against the
monitor gamut
*/
int
clipGamut(float *rgb) {
    // Really SHOULD desaturate instead of just clipping!
    int desaturated = 0;
    for ( int i = 0; i < 3; i++ ) {
        if ( rgb[i] < 0.0 ) {
            rgb[i] = 0.0;
            desaturated = 1;
        }
    }
    return desaturated;
}
