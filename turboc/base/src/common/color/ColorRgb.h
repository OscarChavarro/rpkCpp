#ifndef __COLOR_RGB__
#define __COLOR_RGB__

#include "common/color/ColorRgbMutable.h"

class ColorRgb {
  private:
    float r;
    float g;
    float b;

  public:
    ColorRgb():
        r(0.0f),
        g(0.0f),
        b(0.0f)
    {
    }

    ColorRgb(float inR, float inG, float inB):
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
        return ColorRgbMutable(r, g, b);
    }

    inline float getR() const { return r; }
    inline float getG() const { return g; }
    inline float getB() const { return b; }
    inline float average() const { return (r + g + b) / 3.0f; }
};

#endif
