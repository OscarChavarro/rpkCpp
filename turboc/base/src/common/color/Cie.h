/**
Converts mgf color specification into our representation of colors.
XYZ<->LUV conversions
*/

#ifndef __CIE__
#define __CIE__

#include "common/VSDK.h"

class Cie{ private:
    static float CIE_x_r;
    static float CIE_y_r;
    static float CIE_x_g;
    static float CIE_y_g;
    static float CIE_x_b;
    static float CIE_y_b;
    static float CIE_x_w;
    static float CIE_y_w;

    static float luminousEfficacy;
    static float xyz2RgbMat[3][3];
    static float rgb2XyzMat[3][3];

    static double cieD();
    static double cieCrD();
    static double cieCgD();
    static double cieCbD();
    static double cieRf();
    static double cieGf();
    static double cieBf();

    static float gray(float r, float g, float b);
    static float luminance(float r, float g, float b);

    static void setColorTransform( float mat[3][3], float a, float b, float c, float d, float e, float f, float g, float h, float i);

    static void colorTransform(const float *col, const float mat[3][3], float *res);

  public:
    static const float WHITE_EFFICACY;

    static void transformColorFromXYZ2RGB(const float *xyz, float *rgb);
    static bool clipGamut(float *rgb);

    static void cmptClrConvXforms( float xr, float yr, float xg, float yg, float xb, float yb, float xw, float yw);

    static float getLuminousEfficacy();
    static float spectrumGray(float r, float g, float b);
    static float spectrumLuminance(float r, float g, float b);
};

#endif
