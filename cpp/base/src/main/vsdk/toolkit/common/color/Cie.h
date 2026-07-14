/**
Converts mgf color specification into our representation of colors.
XYZ<->LUV conversions
*/

#ifndef CIE__
#define CIE__

class Cie final {
  private:
    static double CIE_x_r;
    static double CIE_y_r;
    static double CIE_x_g;
    static double CIE_y_g;
    static double CIE_x_b;
    static double CIE_y_b;
    static double CIE_x_w;
    static double CIE_y_w;

    static double luminousEfficacy;
    static double xyz2RgbMat[3][3];
    static double rgb2XyzMat[3][3];

    static double cieD();
    static double cieCrD();
    static double cieCgD();
    static double cieCbD();
    static double cieRf();
    static double cieGf();
    static double cieBf();

    static double gray(double r, double g, double b);
    static double luminance(double r, double g, double b);

    static void setColorTransform(
        double mat[3][3],
        double a, double b, double c,
        double d, double e, double f,
        double g, double h, double i);

    static void colorTransform(const double *col, const double mat[3][3], double *res);

  public:
    static const double WHITE_EFFICACY;

    static void transformColorFromXYZ2RGB(const double *xyz, double *rgb);
    static bool clipGamut(double *rgb);

    static void computeColorConversionTransforms(
        double xr, double yr,
        double xg, double yg,
        double xb, double yb,
        double xw, double yw);

    static double getLuminousEfficacy();
    static double spectrumGray(double r, double g, double b);
    static double spectrumLuminance(double r, double g, double b);
};

#endif
