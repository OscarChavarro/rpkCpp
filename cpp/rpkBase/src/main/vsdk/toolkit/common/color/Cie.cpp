#include "vsdk/toolkit/common/color/Cie.h"

/**
This code is a modified version of the CIE XYZ<->RGB code
in the mgf documentation
*/

namespace {

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
constexpr double WHITE_EFFICACY = 183.07;

}

Cie::Cie(): Cie(
    0.640, 0.330, // Nominal CRT primaries
    0.290, 0.600,
    0.150, 0.060,
    0.3333333333, 0.3333333333)
{
}

/**
Computes RGB <-> XYZ color transforms based on the given monitor primary
colors and white point.
*/
Cie::Cie(
    double xr, double yr,
    double xg, double yg,
    double xb, double yb,
    double xw, double yw):
    xRed(xr),
    yRed(yr),
    xGreen(xg),
    yGreen(yg),
    xBlue(xb),
    yBlue(yb),
    xWhite(xw),
    yWhite(yw),
    luminousEfficacy(WHITE_EFFICACY),
    redGrayFactor(),
    greenGrayFactor(),
    blueGrayFactor(),
    xyz2RgbMat(),
    rgb2XyzMat()
{
    computeColorConversionTransforms();
}

double
Cie::cieD() const {
    return xRed * (yGreen - yBlue) +
           xGreen * (yBlue - yRed) +
           xBlue * (yRed - yGreen);
}

double
Cie::cieCrD() const {
    return (1.0 / yWhite) *
           (xWhite * (yGreen - yBlue) -
            yWhite * (xGreen - xBlue) +
            xGreen * yBlue - xBlue * yGreen);
}

double
Cie::cieCgD() const {
    return (1.0 / yWhite) *
           (xWhite * (yBlue - yRed) -
            yWhite * (xBlue - xRed) -
            xRed * yBlue + xBlue * yRed);
}

double
Cie::cieCbD() const {
    return (1.0 / yWhite) *
           (xWhite * (yRed - yGreen) -
            yWhite * (xRed - xGreen) +
            xRed * yGreen - xGreen * yRed);
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

void
Cie::computeColorConversionTransforms() {
    const double d = cieD();
    const double crD = cieCrD();
    const double cgD = cieCgD();
    const double cbD = cieCbD();

    // Second row of the RGB to XYZ matrix: the Y (gray) of each primary
    redGrayFactor = yRed * crD / d;
    greenGrayFactor = yGreen * cgD / d;
    blueGrayFactor = yBlue * cbD / d;

    setColorTransform(
        xyz2RgbMat, // XYZ to RGB
        (yGreen - yBlue - xBlue * yGreen + yBlue * xGreen) / crD,
        (xBlue - xGreen - xBlue * yGreen + xGreen * yBlue) / crD,
        (xGreen * yBlue - xBlue * yGreen) / crD,
        (yBlue - yRed - yBlue * xRed + yRed * xBlue) / cgD,
        (xRed - xBlue - xRed * yBlue + xBlue * yRed) / cgD,
        (xBlue * yRed - xRed * yBlue) / cgD,
        (yRed - yGreen - yRed * xGreen + yGreen * xRed) / cbD,
        (xGreen - xRed - xGreen * yRed + xRed * yGreen) / cbD,
        (xRed * yGreen - xGreen * yRed) / cbD);

    setColorTransform(
        rgb2XyzMat, // RGB to XYZ
        xRed * crD / d,
        xGreen * cgD / d,
        xBlue * cbD / d,
        redGrayFactor,
        greenGrayFactor,
        blueGrayFactor,
        (1.0 - xRed - yRed) * crD / d,
        (1.0 - xGreen - yGreen) * cgD / d,
        (1.0 - xBlue - yBlue) * cbD / d);
}

/**
CIE XYZ -> RGB
*/
void
Cie::transformColorFromXYZ2RGB(const double *xyz, double *rgb) const {
    colorTransform(xyz, xyz2RgbMat, rgb);
}

/**
RGB -> CIE XYZ
*/
void
Cie::transformColorFromRGB2XYZ(const double *rgb, double *xyz) const {
    colorTransform(rgb, rgb2XyzMat, xyz);
}

/**
Returns an achromatic value representing the spectral quantity.
*/
double
Cie::spectrumGray(double r, double g, double b) const {
    return redGrayFactor * r + greenGrayFactor * g + blueGrayFactor * b;
}

/**
Returns the luminance, photometric quantity corresponding to the
radiance of the given spectrum.
*/
double
Cie::spectrumLuminance(double r, double g, double b) const {
    return luminousEfficacy * spectrumGray(r, g, b);
}
