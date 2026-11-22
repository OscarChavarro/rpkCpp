package vsdk.toolkit.common.color;

/**
Converts mgf color specification into our representation of colors.
XYZ<->LUV conversions
*/

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

// Nominal CRT primaries

/**
Set/return the value used for tri-stimulus white efficacy.
*/

/**
Returns an achromatic value representing the spectral quantity.
*/

/**
Returns the luminance, photometric quantity corresponding to the
radiance of the given spectrum.
*/

/**
Computes RGB <-> XYZ color transforms based on the given monitor primary
colors and white point.
*/

// XYZ to RGB

// RGB to XYZ

/**
CIE XYZ <-> RGB
*/

/**
Returns TRUE if the color was desaturated during clipping against the
monitor gamut
*/

// Really SHOULD desaturate instead of just clipping!

public final class Cie {
    public static final double WHITE_EFFICACY = 183.07;

    private static double cieXR = 0.640; // Nominal CRT primaries
    private static double cieYR = 0.330;
    private static double cieXG = 0.290;
    private static double cieYG = 0.600;
    private static double cieXB = 0.150;
    private static double cieYB = 0.060;
    private static double cieXW = 0.3333333333;
    private static double cieYW = 0.3333333333;

    private static double luminousEfficacy = WHITE_EFFICACY;
    private static final double[][] xyz2RgbMat = new double[3][3];
    private static final double[][] rgb2XyzMat = new double[3][3];

    private Cie() {
    }

    private static double cieD() {
        return cieXR * (cieYG - cieYB) +
               cieXG * (cieYB - cieYR) +
               cieXB * (cieYR - cieYG);
    }

    private static double cieCrD() {
        return (1.0 / cieYW) *
               (cieXW * (cieYG - cieYB) -
                cieYW * (cieXG - cieXB) +
                cieXG * cieYB - cieXB * cieYG);
    }

    private static double cieCgD() {
        return (1.0 / cieYW) *
               (cieXW * (cieYB - cieYR) -
                cieYW * (cieXB - cieXR) -
                cieXR * cieYB + cieXB * cieYR);
    }

    private static double cieCbD() {
        return (1.0 / cieYW) *
               (cieXW * (cieYR - cieYG) -
                cieYW * (cieXR - cieXG) +
                cieXR * cieYG - cieXG * cieYR);
    }

    private static double cieRf() {
        return cieYR * cieCrD() / cieD();
    }

    private static double cieGf() {
        return cieYG * cieCgD() / cieD();
    }

    private static double cieBf() {
        return cieYB * cieCbD() / cieD();
    }

    private static double gray(double r, double g, double b) {
        return cieRf() * r + cieGf() * g + cieBf() * b;
    }

    private static double luminance(double r, double g, double b) {
        return luminousEfficacy * gray(r, g, b);
    }

    private static void setColorTransform(
        double[][] mat,
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

    private static void colorTransform(double[] col, double[][] mat, double[] res) {
        res[0] = mat[0][0] * col[0] + mat[0][1] * col[1] + mat[0][2] * col[2];
        res[1] = mat[1][0] * col[0] + mat[1][1] * col[1] + mat[1][2] * col[2];
        res[2] = mat[2][0] * col[0] + mat[2][1] * col[1] + mat[2][2] * col[2];
    }

    /**
    Set/return the value used for tri-stimulus white efficacy.
    */
    public static double getLuminousEfficacy() {
        return luminousEfficacy;
    }

    /**
    Returns an achromatic value representing the spectral quantity.
    */
    public static double spectrumGray(double r, double g, double b) {
        return gray(r, g, b);
    }

    /**
    Returns the luminance, photometric quantity corresponding to the
    radiance of the given spectrum.
    */
    public static double spectrumLuminance(double r, double g, double b) {
        return luminance(r, g, b);
    }

    /**
    Computes RGB <-> XYZ color transforms based on the given monitor primary
    colors and white point.
    */
    public static void computeColorConversionTransforms(
        double xr,
        double yr,
        double xg,
        double yg,
        double xb,
        double yb,
        double xw,
        double yw)
    {
        cieXR = xr;
        cieYR = yr;
        cieXG = xg;
        cieYG = yg;
        cieXB = xb;
        cieYB = yb;
        cieXW = xw;
        cieYW = yw;

        setColorTransform(
            xyz2RgbMat, // XYZ to RGB
            (cieYG - cieYB - cieXB * cieYG + cieYB * cieXG) / cieCrD(),
            (cieXB - cieXG - cieXB * cieYG + cieXG * cieYB) / cieCrD(),
            (cieXG * cieYB - cieXB * cieYG) / cieCrD(),
            (cieYB - cieYR - cieYB * cieXR + cieYR * cieXB) / cieCgD(),
            (cieXR - cieXB - cieXR * cieYB + cieXB * cieYR) / cieCgD(),
            (cieXB * cieYR - cieXR * cieYB) / cieCgD(),
            (cieYR - cieYG - cieYR * cieXG + cieYG * cieXR) / cieCbD(),
            (cieXG - cieXR - cieXG * cieYR + cieXR * cieYG) / cieCbD(),
            (cieXR * cieYG - cieXG * cieYR) / cieCbD());

        setColorTransform(
            rgb2XyzMat, // RGB to XYZ
            cieXR * cieCrD() / cieD(),
            cieXG * cieCgD() / cieD(),
            cieXB * cieCbD() / cieD(),
            cieYR * cieCrD() / cieD(),
            cieYG * cieCgD() / cieD(),
            cieYB * cieCbD() / cieD(),
            (1.0 - cieXR - cieYR) * cieCrD() / cieD(),
            (1.0 - cieXG - cieYG) * cieCgD() / cieD(),
            (1.0 - cieXB - cieYB) * cieCbD() / cieD());
    }

    /**
    CIE XYZ <-> RGB
    */
    public static void transformColorFromXYZ2RGB(double[] xyz, double[] rgb) {
        colorTransform(xyz, xyz2RgbMat, rgb);
    }

    /**
    Returns TRUE if the color was desaturated during clipping against the
    monitor gamut
    */
    public static boolean clipGamut(double[] rgb) {
        boolean desaturated = false;
        for ( int i = 0; i < 3; i++ ) {
            if ( rgb[i] < 0.0 ) {
                rgb[i] = 0.0;
                desaturated = true;
            }
        }
        return desaturated;
    }
}
