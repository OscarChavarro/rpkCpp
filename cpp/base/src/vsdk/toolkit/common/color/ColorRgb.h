#ifndef COLOR__
#define COLOR__

#include "vsdk/toolkit/java/io/PrintStream.h"

/**
Representation of radiance, radiosity, power, spectra
*/
class ColorRgb {
  private:
    double r;
    double g;
    double b;

  public:
    ColorRgb();
    ColorRgb(double inR, double inG, double inB);

    inline double getR() const { return r; }
    inline double getG() const { return g; }
    inline double getB() const { return b; }

    void clear();
    bool isBlack() const;
    void scaledCopy(double a, ColorRgb c);
    void scale(double a);
    void scalarProduct(ColorRgb s, ColorRgb t);
    void selfScalarProduct(ColorRgb s);
    void scalarProductScaled(ColorRgb s, double a, ColorRgb t);
    void add(ColorRgb s, ColorRgb t);
    void addScaled(ColorRgb s, double a, ColorRgb t);
    void addConstant(ColorRgb s, double a);
    void subtract(ColorRgb s, ColorRgb  t);
    void divide(ColorRgb s, ColorRgb t);
    void scaleInverse(double scale, ColorRgb s);
    double maximumComponent() const;
    double sumAbsComponents() const;
    void abs();
    void maximum(ColorRgb s, ColorRgb t);
    void minimum(ColorRgb s, ColorRgb t);
    double average() const;
    void interpolateBarycentric(ColorRgb c0, ColorRgb c1, ColorRgb c2, double u, double v);
    void interpolateBiLinear(ColorRgb c0, ColorRgb c1, ColorRgb c2, ColorRgb c3, double u, double v);
    void clip();
    void print(java::PrintStream *stream) const;

    static void arrayCopy(ColorRgb *result, const ColorRgb *source, char n);
    static void arrayAdd(ColorRgb *result, const ColorRgb *source, char n);
    static void arrayClear(ColorRgb *color, char n);
};

inline void
ColorRgb::addScaled(const ColorRgb s, const double a, const ColorRgb t) {
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
ColorRgb::scale(const double a) {
    r *= a;
    g *= a;
    b *= a;
}

inline double
ColorRgb::maximumComponent() const {
    if ( r > g ) {
        return r > b ? r : b;
    } else {
        return g > b ? g : b;
    }
}

inline void
ColorRgb::scaledCopy(const double a, const ColorRgb c) {
    r = a * c.r;
    g = a * c.g;
    b = a * c.b;
}

inline void
ColorRgb::divide(const ColorRgb s, const ColorRgb t) {
    r = (t.r != 0.0) ? s.r / t.r : s.r;
    g = (t.g != 0.0) ? s.g / t.g : s.g;
    b = (t.b != 0.0) ? s.b / t.b : s.b;
}

inline void
ColorRgb::scalarProductScaled(const ColorRgb s, const double a, const ColorRgb t) {
    r = s.r * a * t.r;
    g = s.g * a * t.g;
    b = s.b * a * t.b;
}

inline ColorRgb::ColorRgb() {
    r = 0;
    g = 0;
    b = 0;
}

#endif
