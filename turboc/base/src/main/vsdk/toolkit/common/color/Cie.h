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

#ifndef __CIE__
#define __CIE__

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
// C++ port note: no `final` class specifier in the legacy compilers
class Cie {
  private:
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
    // C++ port note: no in-class initializers for static members in the
    // legacy compilers: the value is defined in Cie.cpp
    static const double WHITE_EFFICACY;

    double redX;
    double redY;
    double greenX;
    double greenY;
    double blueX;
    double blueY;
    double whiteX;
    double whiteY;

    double luminousEfficacy;
    double redGrayFactor;
    double greenGrayFactor;
    double blueGrayFactor;
    double xyz2RgbMat[3][3];
    double rgb2XyzMat[3][3];

    double cieD() const;
    double cieCrD() const;
    double cieCgD() const;
    double cieCbD() const;
    void computeColorConversionTransforms();
    // C++ port note: the initialization shared by the constructors (the
    // first one delegates to the second in the cpp port)
    void init(
        double xr, double yr,
        double xg, double yg,
        double xb, double yb,
        double xw, double yw);

    static void setColorTransform(
        double mat[3][3],
        double a, double b, double c,
        double d, double e, double f,
        double g, double h, double i);
    static void colorTransform(const double *col, const double mat[3][3], double *res);

  public:
    // Nominal CRT primaries and an equal-energy white point ([WARD1996] Sec. 6.1.2)
    Cie();
    Cie(double xr, double yr,
        double xg, double yg,
        double xb, double yb,
        double xw, double yw);

    double getRedX() const { return redX; }
    double getRedY() const { return redY; }
    double getGreenX() const { return greenX; }
    double getGreenY() const { return greenY; }
    double getBlueX() const { return blueX; }
    double getBlueY() const { return blueY; }
    double getWhiteX() const { return whiteX; }
    double getWhiteY() const { return whiteY; }
    double getLuminousEfficacy() const { return luminousEfficacy; }

    void transformColorFromXYZ2RGB(const double *xyz, double *rgb) const;
    void transformColorFromRGB2XYZ(const double *rgb, double *xyz) const;
    double spectrumGray(double r, double g, double b) const;
    double spectrumLuminance(double r, double g, double b) const;
};

#endif
