#ifndef __COLOR__
#define __COLOR__

#include <cstdio>
#include <cmath>

#include "common/cie.h"

/**
Representation of radiance, radiosity, power, spectra
*/
class ColorRgb {
  public:
    float r;
    float g;
    float b;

    ColorRgb();
    ColorRgb(float inR, float inG, float inB);

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
    void clip();
    void print(FILE *fp) const;
};

inline ColorRgb::ColorRgb() {
    r = 0;
    g = 0;
    b = 0;
}

inline void
ColorRgb::addScaled(const ColorRgb s, const float a, const ColorRgb t) {
    r = s.r + a * t.r;
    g = s.g + a * t.g;
    b = s.b + a * t.b;
}

inline void
ColorRgb::clear() {
    r = 0;
    g = 0;
    b = 0;
}

inline void
ColorRgb::scale(const float a) {
    r *= a;
    g *= a;
    b *= a;
}

inline float
ColorRgb::maximumComponent() const {
    if ( r > g ) {
        return r > b ? r : b;
    } else {
        return g > b ? g : b;
    }
}

inline void
ColorRgb::scaledCopy(const float a, const ColorRgb c) {
    r = a * c.r;
    g = a * c.g;
    b = a * c.b;
}

inline void
ColorRgb::set(const float v1, const float v2, const float v3) {
    r = v1;
    g = v2;
    b = v3;
}

inline void
ColorRgb::setMonochrome(const float v) {
    r = v;
    g = v;
    b = v;
}

inline void
ColorRgb::divide(const ColorRgb s, const ColorRgb t) {
    r = (t.r != 0.0) ? s.r / t.r : s.r;
    g = (t.g != 0.0) ? s.g / t.g : s.g;
    b = (t.b != 0.0) ? s.b / t.b : s.b;
}

inline void
ColorRgb::scalarProductScaled(const ColorRgb s, const float a, const ColorRgb t) {
    r = s.r * a * t.r;
    g = s.g * a * t.g;
    b = s.b * a * t.b;
}

extern void colorsArrayCopy(ColorRgb *result, const ColorRgb *source, char n);
extern void colorsArrayAdd(ColorRgb *result, const ColorRgb *source, char n);
extern void colorsArrayClear(ColorRgb *color, char n);

#endif
