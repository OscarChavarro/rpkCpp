#include "common/color/Cie.h"

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
const float Cie::WHITE_EFFICACY = 183.07f;

float Cie::CIE_x_r = 0.640f; // Nominal CRT primaries
float Cie::CIE_y_r = 0.330f;
float Cie::CIE_x_g = 0.290f;
float Cie::CIE_y_g = 0.600f;
float Cie::CIE_x_b = 0.150f;
float Cie::CIE_y_b = 0.060f;
float Cie::CIE_x_w = 0.3333333333f;
float Cie::CIE_y_w = 0.3333333333f;

float Cie::luminousEfficacy = Cie::WHITE_EFFICACY;
float Cie::xyz2RgbMat[3][3] = {{0}};
float Cie::rgb2XyzMat[3][3] = {{0}};

double
Cie::cieD() {
    return CIE_x_r * (CIE_y_g - CIE_y_b) +
           CIE_x_g * (CIE_y_b - CIE_y_r) +
           CIE_x_b * (CIE_y_r - CIE_y_g);
}

double
Cie::cieCrD() {
    return (1.0 / CIE_y_w) *
           (CIE_x_w * (CIE_y_g - CIE_y_b) -
            CIE_y_w * (CIE_x_g - CIE_x_b) +
            CIE_x_g * CIE_y_b - CIE_x_b * CIE_y_g);
}

double
Cie::cieCgD() {
    return (1.0 / CIE_y_w) *
           (CIE_x_w * (CIE_y_b - CIE_y_r) -
            CIE_y_w * (CIE_x_b - CIE_x_r) -
            CIE_x_r * CIE_y_b + CIE_x_b * CIE_y_r);
}

double
Cie::cieCbD() {
    return (1.0 / CIE_y_w) *
           (CIE_x_w * (CIE_y_r - CIE_y_g) -
            CIE_y_w * (CIE_x_r - CIE_x_g) +
            CIE_x_r * CIE_y_g - CIE_x_g * CIE_y_r);
}

double
Cie::cieRf() {
    return CIE_y_r * cieCrD() / cieD();
}

double
Cie::cieGf() {
    return CIE_y_g * cieCgD() / cieD();
}

double
Cie::cieBf() {
    return CIE_y_b * cieCbD() / cieD();
}

float
Cie::gray(float r, float g, float b) {
    return ((float)(cieRf())) * r + ((float)(cieGf())) * g + ((float)(cieBf())) * b;
}

float
Cie::luminance(float r, float g, float b) {
    return luminousEfficacy * gray(r, g, b);
}

void
Cie::setColorTransform(
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

void
Cie::colorTransform(const float *col, const float mat[3][3], float *res) {
    res[0] = mat[0][0] * col[0] + mat[0][1] * col[1] + mat[0][2] * col[2];
    res[1] = mat[1][0] * col[0] + mat[1][1] * col[1] + mat[1][2] * col[2];
    res[2] = mat[2][0] * col[0] + mat[2][1] * col[1] + mat[2][2] * col[2];
}

/**
Set/return the value used for tri-stimulus white efficacy.
*/
float
Cie::getLuminousEfficacy() {
    return luminousEfficacy;
}

/**
Returns an achromatic value representing the spectral quantity.
*/
float
Cie::spectrumGray(float r, float g, float b) {
    return gray(r, g, b);
}

/**
Returns the luminance, photometric quantity corresponding to the
radiance of the given spectrum.
*/
float
Cie::spectrumLuminance(float r, float g, float b) {
    return luminance(r, g, b);
}

/**
Computes RGB <-> XYZ color transforms based on the given monitor primary
colors and white point.
*/
void
Cie::cmptClrConvXforms(
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
            xyz2RgbMat, // XYZ to RGB
          ((float)((CIE_y_g - CIE_y_b - CIE_x_b * CIE_y_g + CIE_y_b * CIE_x_g) / cieCrD())),
            ((float)((CIE_x_b - CIE_x_g - CIE_x_b * CIE_y_g + CIE_x_g * CIE_y_b) / cieCrD())),
            ((float)((CIE_x_g * CIE_y_b - CIE_x_b * CIE_y_g) / cieCrD())),
            ((float)((CIE_y_b - CIE_y_r - CIE_y_b * CIE_x_r + CIE_y_r * CIE_x_b) / cieCgD())),
            ((float)((CIE_x_r - CIE_x_b - CIE_x_r * CIE_y_b + CIE_x_b * CIE_y_r) / cieCgD())),
            ((float)((CIE_x_b * CIE_y_r - CIE_x_r * CIE_y_b) / cieCgD())),
            ((float)((CIE_y_r - CIE_y_g - CIE_y_r * CIE_x_g + CIE_y_g * CIE_x_r) / cieCbD())),
            ((float)((CIE_x_g - CIE_x_r - CIE_x_g * CIE_y_r + CIE_x_r * CIE_y_g) / cieCbD())),
            ((float)((CIE_x_r * CIE_y_g - CIE_x_g * CIE_y_r) / cieCbD())));

    setColorTransform(
            rgb2XyzMat, // RGB to XYZ
          ((float)(CIE_x_r * cieCrD() / cieD())),
            ((float)(CIE_x_g * cieCgD() / cieD())),
            ((float)(CIE_x_b * cieCbD() / cieD())),
            ((float)(CIE_y_r * cieCrD() / cieD())),
            ((float)(CIE_y_g * cieCgD() / cieD())),
            ((float)(CIE_y_b * cieCbD() / cieD())),
            ((float)((1.0 - CIE_x_r - CIE_y_r) * cieCrD() / cieD())),
            ((float)((1.0 - CIE_x_g - CIE_y_g) * cieCgD() / cieD())),
            ((float)((1.0 - CIE_x_b - CIE_y_b) * cieCbD() / cieD())));
}

/**
CIE XYZ <-> RGB
*/
void
Cie::transformColorFromXYZ2RGB(const float *xyz, float *rgb) {
    colorTransform(xyz, xyz2RgbMat, rgb);
}

/**
Returns TRUE if the color was desaturated during clipping against the
monitor gamut
*/
bool
Cie::clipGamut(float *rgb) {
    // Really SHOULD desaturate instead of just clipping!
    bool desaturated = false;
    for ( int i = 0; i < 3; i++ ) {
        if ( rgb[i] < 0.0 ) {
            rgb[i] = 0.0;
            desaturated = true;
        }
    }
    return desaturated;
}
