#ifndef __COLOR_MUTABLE__
#define __COLOR_MUTABLE__

#include "java/io/PrintStream.h"

class ColorRgbMutable {
  private:
    float r;
    float g;
    float b;

  public:
    ColorRgbMutable();
    ColorRgbMutable(float inR, float inG, float inB);

    inline float getR() const { return r; }
    inline float getG() const { return g; }
    inline float getB() const { return b; }

    void clear();
    bool isBlack() const;
    void scaledCopy(float a, ColorRgbMutable c);
    void scale(float a);
    void scalarProduct(ColorRgbMutable s, ColorRgbMutable t);
    void selfScalarProduct(ColorRgbMutable s);
    void scalarProductScaled(ColorRgbMutable s, float a, ColorRgbMutable t);
    void add(ColorRgbMutable s, ColorRgbMutable t);
    void addScaled(ColorRgbMutable s, float a, ColorRgbMutable t);
    void addConstant(ColorRgbMutable s, float a);
    void subtract(ColorRgbMutable s, ColorRgbMutable t);
    void divide(ColorRgbMutable s, ColorRgbMutable t);
    void scaleInverse(float scale, ColorRgbMutable s);
    float maximumComponent() const;
    float sumAbsComponents() const;
    void abs();
    void maximum(ColorRgbMutable s, ColorRgbMutable t);
    void minimum(ColorRgbMutable s, ColorRgbMutable t);
    float average() const;
    void interpolateBarycentric(ColorRgbMutable c0, ColorRgbMutable c1, ColorRgbMutable c2, float u, float v);
    void interpolateBiLinear(ColorRgbMutable c0, ColorRgbMutable c1, ColorRgbMutable c2, ColorRgbMutable c3, float u, float v);
    void clip();
    void print(PrintStream *stream) const;

    static void arrayCopy(ColorRgbMutable *result, const ColorRgbMutable *source, char n);
    static void arrayAdd(ColorRgbMutable *result, const ColorRgbMutable *source, char n);
    static void arrayClear(ColorRgbMutable *color, char n);
};

inline ColorRgbMutable::ColorRgbMutable() {
    r = 0;
    g = 0;
    b = 0;
}

inline void
ColorRgbMutable::addScaled(const ColorRgbMutable s, const float a, const ColorRgbMutable t) {
    r = s.r + a * t.r;
    g = s.g + a * t.g;
    b = s.b + a * t.b;
}

inline void
ColorRgbMutable::clear() {
    r = 0;
    g = 0;
    b = 0;
}

inline void
ColorRgbMutable::scale(const float a) {
    r *= a;
    g *= a;
    b *= a;
}

inline float
ColorRgbMutable::maximumComponent() const {
    if ( r > g ) {
        return r > b ? r : b;
    } else {
        return g > b ? g : b;
    }
}

inline void
ColorRgbMutable::scaledCopy(const float a, const ColorRgbMutable c) {
    r = a * c.r;
    g = a * c.g;
    b = a * c.b;
}

inline void
ColorRgbMutable::divide(const ColorRgbMutable s, const ColorRgbMutable t) {
    r = (t.r != 0.0f) ? s.r / t.r : s.r;
    g = (t.g != 0.0f) ? s.g / t.g : s.g;
    b = (t.b != 0.0f) ? s.b / t.b : s.b;
}

inline void
ColorRgbMutable::scalarProductScaled(const ColorRgbMutable s, const float a, const ColorRgbMutable t) {
    r = s.r * a * t.r;
    g = s.g * a * t.g;
    b = s.b * a * t.b;
}

#endif
