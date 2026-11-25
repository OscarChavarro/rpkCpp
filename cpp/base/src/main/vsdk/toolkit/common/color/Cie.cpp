//= References:                                                             =
//= [WARD1996] Ward, Greg. "The Materials and Geometry Format", version     =
//=     1.0. Lawrence Berkeley Laboratory, February 1996. Section 6.1.2,    =
//=     "Converting between RGB and CIE Colors".                            =
//=     PDF: doc/references/[WARD1996]_*.pdf                                =
//= [COHE1993] Cohen, Michael F. and Wallace, John R. "Radiosity and        =
//=     Realistic Image Synthesis". Academic Press Professional, 1993.      =
//=     Sections 2.3, 9.5.2, 9.6.2 and 9.6.3.                               =
//=     PDF: doc/references/[COHE1993]_*.pdf                                =
//= [ARVO1991] Arvo, James (editor). "Graphics Gems II". Academic Press,    =
//=     1991.                                                               =
//=     PDF: doc/references/[ARVO1991]_*.pdf                                =
//= [MART1991] Martindale, David and Paeth, Alan W. "Television Color       =
//=     Encoding and 'Hot' Broadcast Colors". In [ARVO1991], gem III.6,     =
//=     pp. 147-158.                                                        =
//= [MEYE1991] Meyer, Gary W. "An Inexpensive Method of Setting the         =
//=     Monitor White Point". In [ARVO1991], gem III.7, pp. 159-162.        =
//= [TUMB1993] Tumblin, Jack and Rushmeier, Holly. "Tone Reproduction for   =
//=     Realistic Images". IEEE Computer Graphics and Applications 13(6),   =
//=     November 1993, pp. 42-48.                                           =
//=     PDF: doc/references/[TUMB1993]_*.pdf                                =
//= [FERW1996] Ferwerda, James A., Pattanaik, Sumanta N., Shirley, Peter    =
//=     and Greenberg, Donald P. "A Model of Visual Adaptation for          =
//=     Realistic Image Synthesis". SIGGRAPH '96, 1996, pp. 249-258.        =
//=     PDF: doc/references/[FERW1996]_*.pdf                                =

#include "vsdk/toolkit/common/color/Cie.h"

/**
This code is a modified version of the CIE XYZ<->RGB code
in the mgf documentation ([WARD1996] Sec. 6.1.2)
*/

/**
The "nominal CRT primaries" and the monitor white point of [WARD1996]
Sec. 6.1.2 (its non-NTSC branch).
*/
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

/**
`CIE_D` of [WARD1996] Sec. 6.1.2: the determinant of the matrix whose
columns are the chromaticities of the three primaries.
*/
double
Cie::cieD() const {
    return xRed * (yGreen - yBlue) +
           xGreen * (yBlue - yRed) +
           xBlue * (yRed - yGreen);
}

/**
`CIE_C_rD` of [WARD1996] Sec. 6.1.2. `cieCrD() / cieD()` is the unknown
J_r of the white point system of [MART1991].
*/
double
Cie::cieCrD() const {
    return (1.0 / yWhite) *
           (xWhite * (yGreen - yBlue) -
            yWhite * (xGreen - xBlue) +
            xGreen * yBlue - xBlue * yGreen);
}

/**
`CIE_C_gD` of [WARD1996] Sec. 6.1.2 (J_g of [MART1991] times `cieD()`).
*/
double
Cie::cieCgD() const {
    return (1.0 / yWhite) *
           (xWhite * (yBlue - yRed) -
            yWhite * (xBlue - xRed) -
            xRed * yBlue + xBlue * yRed);
}

/**
`CIE_C_bD` of [WARD1996] Sec. 6.1.2 (J_b of [MART1991] times `cieD()`).
*/
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

/**
Builds the `xyz2rgbmat` and `rgb2xyzmat` matrices of [WARD1996] Sec. 6.1.2.

Each column of the RGB to XYZ matrix is the chromaticity (x, y, z) of a
primary, with z = 1 - x - y ([COHE1993] Eq. 9.21), scaled by the amount
J = C_D / D of that primary in the white point. [MART1991] finds the J's by
solving

    | x_r x_g x_b |   | J_r |   | x_w / y_w |
    | y_r y_g y_b | * | J_g | = |     1     |
    | z_r z_g z_b |   | J_b |   | z_w / y_w |

and gives J_r * y_r, J_g * y_g and J_b * y_b as the relative luminance
contributions of the primaries to the reference white: the second row of
the matrix, kept as the gray factors. [MART1991] warns that the familiar
weights 0.299, 0.587 and 0.114 hold only for the NTSC primaries with the
Illuminant C white point, which is why they are computed here from the
chromaticities.
*/
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
CIE XYZ -> RGB, `cie_rgb` of [WARD1996] Sec. 6.1.2. Unlike `cie_rgb`, the
negative components of a color outside the gamut of the primaries (outside
their triangle in the chromaticity diagram, [COHE1993] Fig. 9.28) are not
clamped to zero: clamping is left to the caller.
*/
void
Cie::transformColorFromXYZ2RGB(const double *xyz, double *rgb) const {
    colorTransform(xyz, xyz2RgbMat, rgb);
}

/**
RGB -> CIE XYZ, `rgb_cie` of [WARD1996] Sec. 6.1.2.
*/
void
Cie::transformColorFromRGB2XYZ(const double *rgb, double *xyz) const {
    colorTransform(rgb, rgb2XyzMat, xyz);
}

/**
Returns an achromatic value representing the spectral quantity: the CIE Y
of the color, whose matching function is the luminous efficiency function
([COHE1993] Sec. 9.6.2). [COHE1993] Sec. 9.6.3 proposes this Y channel as
the luminance measure for error metrics.
*/
double
Cie::spectrumGray(double r, double g, double b) const {
    return redGrayFactor * r + greenGrayFactor * g + blueGrayFactor * b;
}

/**
Returns the luminance, photometric quantity corresponding to the
radiance of the given spectrum ([COHE1993] Sec. 2.3).
*/
double
Cie::spectrumLuminance(double r, double g, double b) const {
    return luminousEfficacy * spectrumGray(r, g, b);
}
