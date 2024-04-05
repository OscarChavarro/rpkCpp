#ifndef __COLOR__
#define __COLOR__

#include "common/cie.h"
#include "common/rgb.h"

/**
Representation of radiance, radiosity, power, spectra
*/
class ColorRgb {
  public:
    float spectrum[3];
    ColorRgb();

    inline void
    print(FILE *fp) {
        fprintf(fp, "%g %g %g", spectrum[0], spectrum[1], spectrum[2]);
    }

    void clear();
    void set(float v1, float v2, float v3);
    void setMonochrome(float v);
    bool isBlack() const;
    void scaledCopy(float a, ColorRgb c);
    void scale(float a);
    void scalarProduct(ColorRgb s, ColorRgb t);
    void selfScalarProduct(ColorRgb s);
    void scalarProductScaled(ColorRgb s, float a, ColorRgb t);
    void add(ColorRgb s, ColorRgb t);
    void addScaled(ColorRgb s, float a, ColorRgb t);
    void addConstant(ColorRgb s, float a);
    void subtract(ColorRgb s, ColorRgb  t);
    void divide(ColorRgb s, ColorRgb t);
    void scaleInverse(float scale, ColorRgb s);
    float maximumComponent() const;
    float sumAbsComponents() const;
    void abs();
    void maximum(ColorRgb s, ColorRgb t);
    void minimum(ColorRgb s, ColorRgb t);
    float average() const;
    float gray() const;
    float luminance() const;
    void interpolateBarycentric(ColorRgb c0, ColorRgb c1, ColorRgb c2, float u, float v);
    void interpolateBiLinear(ColorRgb c0, ColorRgb c1, ColorRgb c2, ColorRgb c3, float u, float v);
};

extern RGB *convertColorToRGB(ColorRgb col, RGB *rgb);
extern ColorRgb *convertRGBToColor(RGB rgb, ColorRgb *col);

#endif
