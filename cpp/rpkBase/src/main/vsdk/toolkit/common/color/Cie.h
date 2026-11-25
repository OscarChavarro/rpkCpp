/**
An RGB color space defined by the CIE xy chromaticities of its three
primaries and of its white point: CIE XYZ <-> RGB conversion and the
achromatic (gray) and photometric (luminance) values of an RGB triple.

Instances are immutable values. Whoever selects the display primaries (the
tone mapping context) owns one and hands it to the code that needs it.
*/

#ifndef CIE__
#define CIE__

class Cie final {
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
    */
    static constexpr double WHITE_EFFICACY = 183.07;

    double xRed;
    double yRed;
    double xGreen;
    double yGreen;
    double xBlue;
    double yBlue;
    double xWhite;
    double yWhite;

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

    static void setColorTransform(
        double mat[3][3],
        double a, double b, double c,
        double d, double e, double f,
        double g, double h, double i);
    static void colorTransform(const double *col, const double mat[3][3], double *res);

  public:
    // Nominal CRT primaries and an equal-energy white point
    Cie();
    Cie(double xr, double yr,
        double xg, double yg,
        double xb, double yb,
        double xw, double yw);

    double getRedX() const { return xRed; }
    double getRedY() const { return yRed; }
    double getGreenX() const { return xGreen; }
    double getGreenY() const { return yGreen; }
    double getBlueX() const { return xBlue; }
    double getBlueY() const { return yBlue; }
    double getWhiteX() const { return xWhite; }
    double getWhiteY() const { return yWhite; }
    double getLuminousEfficacy() const { return luminousEfficacy; }

    void transformColorFromXYZ2RGB(const double *xyz, double *rgb) const;
    void transformColorFromRGB2XYZ(const double *rgb, double *xyz) const;
    double spectrumGray(double r, double g, double b) const;
    double spectrumLuminance(double r, double g, double b) const;
};

#endif
