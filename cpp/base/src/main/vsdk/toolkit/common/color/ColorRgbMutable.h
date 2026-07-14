#ifndef COLOR__
#define COLOR__

#include "java/io/PrintStream.h"

/**
Representation of radiance, radiosity, power, spectra
*/
class ColorRgbMutable {
  private:
    double r;
    double g;
    double b;

  public:
    ColorRgbMutable();
    ColorRgbMutable(double inR, double inG, double inB);

    inline double getR() const { return r; }
    inline double getG() const { return g; }
    inline double getB() const { return b; }

    void clear();
    bool isBlack() const;
    void scaledCopy(double a, ColorRgbMutable c);
    void scale(double a);
    void scalarProduct(ColorRgbMutable s, ColorRgbMutable t);
    void selfScalarProduct(ColorRgbMutable s);
    void scalarProductScaled(ColorRgbMutable s, double a, ColorRgbMutable t);
    void add(ColorRgbMutable s, ColorRgbMutable t);
    void addScaled(ColorRgbMutable s, double a, ColorRgbMutable t);
    void addConstant(ColorRgbMutable s, double a);
    void subtract(ColorRgbMutable s, ColorRgbMutable  t);
    void divide(ColorRgbMutable s, ColorRgbMutable t);
    void scaleInverse(double scale, ColorRgbMutable s);
    double maximumComponent() const;
    double sumAbsComponents() const;
    void abs();
    void maximum(ColorRgbMutable s, ColorRgbMutable t);
    void minimum(ColorRgbMutable s, ColorRgbMutable t);
    double average() const;
    void interpolateBarycentric(ColorRgbMutable c0, ColorRgbMutable c1, ColorRgbMutable c2, double u, double v);
    void interpolateBiLinear(ColorRgbMutable c0, ColorRgbMutable c1, ColorRgbMutable c2, ColorRgbMutable c3, double u, double v);
    void clip();
    void print(java::PrintStream *stream) const;

    static void arrayCopy(ColorRgbMutable *result, const ColorRgbMutable *source, char n);
    static void arrayAdd(ColorRgbMutable *result, const ColorRgbMutable *source, char n);
    static void arrayClear(ColorRgbMutable *color, char n);
};

inline void
ColorRgbMutable::addScaled(const ColorRgbMutable s, const double a, const ColorRgbMutable t) {
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
ColorRgbMutable::scale(const double a) {
    r *= a;
    g *= a;
    b *= a;
}

inline double
ColorRgbMutable::maximumComponent() const {
    if ( r > g ) {
        return r > b ? r : b;
    } else {
        return g > b ? g : b;
    }
}

inline void
ColorRgbMutable::scaledCopy(const double a, const ColorRgbMutable c) {
    r = a * c.r;
    g = a * c.g;
    b = a * c.b;
}

inline void
ColorRgbMutable::divide(const ColorRgbMutable s, const ColorRgbMutable t) {
    r = (t.r != 0.0) ? s.r / t.r : s.r;
    g = (t.g != 0.0) ? s.g / t.g : s.g;
    b = (t.b != 0.0) ? s.b / t.b : s.b;
}

inline void
ColorRgbMutable::scalarProductScaled(const ColorRgbMutable s, const double a, const ColorRgbMutable t) {
    r = s.r * a * t.r;
    g = s.g * a * t.g;
    b = s.b * a * t.b;
}

inline ColorRgbMutable::ColorRgbMutable() {
    r = 0;
    g = 0;
    b = 0;
}

#endif
