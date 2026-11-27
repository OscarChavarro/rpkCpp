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

package vsdk.toolkit.common.color;

/**
An RGB color space defined by the CIE xy chromaticities of its three
primaries and of its white point: CIE XYZ <-> RGB conversion and the
achromatic (gray) and photometric (luminance) values of an RGB triple.

Instances are immutable values. Whoever selects the display primaries (the
tone mapping context) owns one and hands it to the code that needs it.

Traceability:
- The conversion code is a modified version of the CIE XYZ <-> RGB code of
  [WARD1996] Sec. 6.1.2: `cieD`, `cieCrD`, `cieCgD` and `cieCbD` are its
  `CIE_D`, `CIE_C_rD`, `CIE_C_gD` and `CIE_C_bD` macros, the gray factors
  are its `CIE_rf`, `CIE_gf` and `CIE_bf`, the two matrices are its
  `xyz2rgbmat` and `rgb2xyzmat`, and the two transforms are its `cie_rgb`
  and `rgb_cie`. Its default primaries are the "nominal CRT primaries" of
  that code; its `NTSC` variant is obtained with the eight-value
  constructor. MGF itself specifies colors as CIE (x, y) chromaticities of
  the 1931 CIE 2 degree standard observer (the `cxy` entity), which is why
  an MGF reader needs this class.
- The CIE XYZ space, the chromaticity coordinates x = X / (X + Y + Z),
  y = Y / (X + Y + Z) and the monitor gamut as a triangle in the
  chromaticity diagram are those of [COHE1993] Sec. 9.6.2 (Eq. 9.21,
  Figs. 9.28 and 9.29). [COHE1993] Sec. 9.6.3 explains why the XYZ <-> RGB
  transform depends on the particular monitor, and so why the primaries are
  parameters of an instance and not global constants.
- The luminance weights of the primaries (the gray factors) are derived
  from the primaries and the white point as in [MART1991]; the white point
  itself is discussed in [MEYE1991].
- Tone mapping operators such as [TUMB1993] and [FERW1996] (see also
  [COHE1993] Sec. 9.5.2) are the consumers of `spectrumLuminance` and
  `getLuminousEfficacy`: they need the real-world luminance of a radiance.
*/
public final class Cie
{
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

    The integral over the luminous efficiency function is Eq. 2.1 of
    [COHE1993] Sec. 2.3, which gives the peak efficacy as 684 lm/W at 555 nm;
    [WARD1996] uses 683 lm/W as its peak lumens per watt multiplier
    (`C_CMAXV`). The value 183.07 below comes from neither reference.
    */
    private static final double WHITE_EFFICACY = 183.07;

    private final double redX;
    private final double redY;
    private final double greenX;
    private final double greenY;
    private final double blueX;
    private final double blueY;
    private final double whiteX;
    private final double whiteY;

    private final double luminousEfficacy;
    private double redGrayFactor;
    private double greenGrayFactor;
    private double blueGrayFactor;
    private final double[][] xyz2RgbMat;
    private final double[][] rgb2XyzMat;

    /**
    The "nominal CRT primaries" and the monitor white point of [WARD1996]
    Sec. 6.1.2 (its non-NTSC branch).
    */
    public Cie()
    {
        this(
            0.640, 0.330, // Nominal CRT primaries
            0.290, 0.600,
            0.150, 0.060,
            0.3333333333, 0.3333333333);
    }

    /**
    Computes RGB <-> XYZ color transforms based on the given monitor primary
    colors and white point.
    @param xr x chromaticity of the red primary
    @param yr y chromaticity of the red primary
    @param xg x chromaticity of the green primary
    @param yg y chromaticity of the green primary
    @param xb x chromaticity of the blue primary
    @param yb y chromaticity of the blue primary
    @param xw x chromaticity of the white point
    @param yw y chromaticity of the white point
    */
    public Cie(
        double xr, double yr,
        double xg, double yg,
        double xb, double yb,
        double xw, double yw)
    {
        redX = xr;
        redY = yr;
        greenX = xg;
        greenY = yg;
        blueX = xb;
        blueY = yb;
        whiteX = xw;
        whiteY = yw;
        luminousEfficacy = WHITE_EFFICACY;
        redGrayFactor = 0.0;
        greenGrayFactor = 0.0;
        blueGrayFactor = 0.0;
        xyz2RgbMat = new double[3][3];
        rgb2XyzMat = new double[3][3];
        computeColorConversionTransforms();
    }

    public double getRedX() { return redX; }
    public double getRedY() { return redY; }
    public double getGreenX() { return greenX; }
    public double getGreenY() { return greenY; }
    public double getBlueX() { return blueX; }
    public double getBlueY() { return blueY; }
    public double getWhiteX() { return whiteX; }
    public double getWhiteY() { return whiteY; }
    public double getLuminousEfficacy() { return luminousEfficacy; }

    /**
    `CIE_D` of [WARD1996] Sec. 6.1.2: the determinant of the matrix whose
    columns are the chromaticities of the three primaries.
    */
    private double cieD()
    {
        return redX * (greenY - blueY) +
               greenX * (blueY - redY) +
               blueX * (redY - greenY);
    }

    /**
    `CIE_C_rD` of [WARD1996] Sec. 6.1.2. `cieCrD() / cieD()` is the unknown
    J_r of the white point system of [MART1991].
    */
    private double cieCrD()
    {
        return (1.0 / whiteY) *
               (whiteX * (greenY - blueY) -
                whiteY * (greenX - blueX) +
                greenX * blueY - blueX * greenY);
    }

    /**
    `CIE_C_gD` of [WARD1996] Sec. 6.1.2 (J_g of [MART1991] times `cieD()`).
    */
    private double cieCgD()
    {
        return (1.0 / whiteY) *
               (whiteX * (blueY - redY) -
                whiteY * (blueX - redX) -
                redX * blueY + blueX * redY);
    }

    /**
    `CIE_C_bD` of [WARD1996] Sec. 6.1.2 (J_b of [MART1991] times `cieD()`).
    */
    private double cieCbD()
    {
        return (1.0 / whiteY) *
               (whiteX * (redY - greenY) -
                whiteY * (redX - greenX) +
                redX * greenY - greenX * redY);
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

    private static void colorTransform(double[] col, double[][] mat,
        double[] res)
    {
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
    private void computeColorConversionTransforms()
    {
        final double d = cieD();
        final double crD = cieCrD();
        final double cgD = cieCgD();
        final double cbD = cieCbD();

        // Second row of the RGB to XYZ matrix: the Y (gray) of each primary
        redGrayFactor = redY * crD / d;
        greenGrayFactor = greenY * cgD / d;
        blueGrayFactor = blueY * cbD / d;

        setColorTransform(
            xyz2RgbMat, // XYZ to RGB
            (greenY - blueY - blueX * greenY + blueY * greenX) / crD,
            (blueX - greenX - blueX * greenY + greenX * blueY) / crD,
            (greenX * blueY - blueX * greenY) / crD,
            (blueY - redY - blueY * redX + redY * blueX) / cgD,
            (redX - blueX - redX * blueY + blueX * redY) / cgD,
            (blueX * redY - redX * blueY) / cgD,
            (redY - greenY - redY * greenX + greenY * redX) / cbD,
            (greenX - redX - greenX * redY + redX * greenY) / cbD,
            (redX * greenY - greenX * redY) / cbD);

        setColorTransform(
            rgb2XyzMat, // RGB to XYZ
            redX * crD / d,
            greenX * cgD / d,
            blueX * cbD / d,
            redGrayFactor,
            greenGrayFactor,
            blueGrayFactor,
            (1.0 - redX - redY) * crD / d,
            (1.0 - greenX - greenY) * cgD / d,
            (1.0 - blueX - blueY) * cbD / d);
    }

    /**
    CIE XYZ -> RGB, `cie_rgb` of [WARD1996] Sec. 6.1.2. Unlike `cie_rgb`, the
    negative components of a color outside the gamut of the primaries (outside
    their triangle in the chromaticity diagram, [COHE1993] Fig. 9.28) are not
    clamped to zero: clamping is left to the caller.
    @param xyz input CIE XYZ triple (three elements)
    @param rgb output RGB triple (three elements)
    */
    public void transformColorFromXYZ2RGB(double[] xyz, double[] rgb)
    {
        colorTransform(xyz, xyz2RgbMat, rgb);
    }

    /**
    RGB -> CIE XYZ, `rgb_cie` of [WARD1996] Sec. 6.1.2.
    @param rgb input RGB triple (three elements)
    @param xyz output CIE XYZ triple (three elements)
    */
    public void transformColorFromRGB2XYZ(double[] rgb, double[] xyz)
    {
        colorTransform(rgb, rgb2XyzMat, xyz);
    }

    /**
    Returns an achromatic value representing the spectral quantity: the CIE Y
    of the color, whose matching function is the luminous efficiency function
    ([COHE1993] Sec. 9.6.2). [COHE1993] Sec. 9.6.3 proposes this Y channel as
    the luminance measure for error metrics.
    */
    public double spectrumGray(double r, double g, double b)
    {
        return redGrayFactor * r + greenGrayFactor * g + blueGrayFactor * b;
    }

    /**
    Returns the luminance, photometric quantity corresponding to the
    radiance of the given spectrum ([COHE1993] Sec. 2.3).
    */
    public double spectrumLuminance(double r, double g, double b)
    {
        return luminousEfficacy * spectrumGray(r, g, b);
    }
}
