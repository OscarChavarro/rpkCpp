#include "vsdk/toolkit/common/color/Cie.h"

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
const double Cie::WHITE_EFFICACY = 183.07;

double Cie::CIE_x_r = 0.640; // Nominal CRT primaries
double Cie::CIE_y_r = 0.330;
double Cie::CIE_x_g = 0.290;
double Cie::CIE_y_g = 0.600;
double Cie::CIE_x_b = 0.150;
double Cie::CIE_y_b = 0.060;
double Cie::CIE_x_w = 0.3333333333;
double Cie::CIE_y_w = 0.3333333333;

double Cie::luminousEfficacy = Cie::WHITE_EFFICACY;
double Cie::xyz2RgbMat[3][3] = {};
double Cie::rgb2XyzMat[3][3] = {};

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

double
Cie::gray(double r, double g, double b) {
    return cieRf() * r + cieGf() * g + cieBf() * b;
}

double
Cie::luminance(double r, double g, double b) {
    return luminousEfficacy * gray(r, g, b);
}

void
Cie::setColorTransform(
    double mat[3][3],
    double a, double b, double c,
    double d, double e, double f,
    double g, double h, double i)
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
Cie::colorTransform(const double *col, const double mat[3][3], double *res) {
    res[0] = mat[0][0] * col[0] + mat[0][1] * col[1] + mat[0][2] * col[2];
    res[1] = mat[1][0] * col[0] + mat[1][1] * col[1] + mat[1][2] * col[2];
    res[2] = mat[2][0] * col[0] + mat[2][1] * col[1] + mat[2][2] * col[2];
}

/**
Set/return the value used for tri-stimulus white efficacy.
*/
double
Cie::getLuminousEfficacy() {
    return luminousEfficacy;
}

/**
Returns an achromatic value representing the spectral quantity.
*/
double
Cie::spectrumGray(double r, double g, double b) {
    return gray(r, g, b);
}

/**
Returns the luminance, photometric quantity corresponding to the
radiance of the given spectrum.
*/
double
Cie::spectrumLuminance(double r, double g, double b) {
    return luminance(r, g, b);
}

/**
Computes RGB <-> XYZ color transforms based on the given monitor primary
colors and white point.
*/
void
Cie::computeColorConversionTransforms(
    double xr,
    double yr,
    double xg,
    double yg,
    double xb,
    double yb,
    double xw,
    double yw)
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
          (CIE_y_g - CIE_y_b - CIE_x_b * CIE_y_g + CIE_y_b * CIE_x_g) / cieCrD(),
            (CIE_x_b - CIE_x_g - CIE_x_b * CIE_y_g + CIE_x_g * CIE_y_b) / cieCrD(),
            (CIE_x_g * CIE_y_b - CIE_x_b * CIE_y_g) / cieCrD(),
            (CIE_y_b - CIE_y_r - CIE_y_b * CIE_x_r + CIE_y_r * CIE_x_b) / cieCgD(),
            (CIE_x_r - CIE_x_b - CIE_x_r * CIE_y_b + CIE_x_b * CIE_y_r) / cieCgD(),
            (CIE_x_b * CIE_y_r - CIE_x_r * CIE_y_b) / cieCgD(),
            (CIE_y_r - CIE_y_g - CIE_y_r * CIE_x_g + CIE_y_g * CIE_x_r) / cieCbD(),
            (CIE_x_g - CIE_x_r - CIE_x_g * CIE_y_r + CIE_x_r * CIE_y_g) / cieCbD(),
            (CIE_x_r * CIE_y_g - CIE_x_g * CIE_y_r) / cieCbD());

    setColorTransform(
            rgb2XyzMat, // RGB to XYZ
          CIE_x_r * cieCrD() / cieD(),
            CIE_x_g * cieCgD() / cieD(),
            CIE_x_b * cieCbD() / cieD(),
            CIE_y_r * cieCrD() / cieD(),
            CIE_y_g * cieCgD() / cieD(),
            CIE_y_b * cieCbD() / cieD(),
            (1.0 - CIE_x_r - CIE_y_r) * cieCrD() / cieD(),
            (1.0 - CIE_x_g - CIE_y_g) * cieCgD() / cieD(),
            (1.0 - CIE_x_b - CIE_y_b) * cieCbD() / cieD());
}

/**
CIE XYZ <-> RGB
*/
void
Cie::transformColorFromXYZ2RGB(const double *xyz, double *rgb) {
    colorTransform(xyz, xyz2RgbMat, rgb);
}

/**
Returns TRUE if the color was desaturated during clipping against the
monitor gamut
*/
bool
Cie::clipGamut(double *rgb) {
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
