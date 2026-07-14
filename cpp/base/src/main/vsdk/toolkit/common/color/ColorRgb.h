#ifndef COLOR_RGB__
#define COLOR_RGB__

#include "vsdk/toolkit/common/color/ColorRgbMutable.h"

class ColorRgb {
  private:
    double r;
    double g;
    double b;

  public:
    ColorRgb():
        r(0.0),
        g(0.0),
        b(0.0)
    {
    }

    ColorRgb(double inR, double inG, double inB):
        r(inR),
        g(inG),
        b(inB)
    {
    }

    explicit ColorRgb(const ColorRgbMutable &c):
        r(c.getR()),
        g(c.getG()),
        b(c.getB())
    {
    }

    operator ColorRgbMutable() const {
        return {r, g, b};
    }

    inline double getR() const { return r; }
    inline double getG() const { return g; }
    inline double getB() const { return b; }
    inline double average() const { return (r + g + b) / 3.0; }
};

#endif
