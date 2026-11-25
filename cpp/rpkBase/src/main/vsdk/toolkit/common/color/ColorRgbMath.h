#ifndef COLOR_RGB_MATH__
#define COLOR_RGB_MATH__

#include "vsdk/toolkit/common/color/ColorRgb.h"

class ColorRgbMath final {
  public:
    static ColorRgb add(const ColorRgb &s, const ColorRgb &t) {
        return ColorRgb(s.getR() + t.getR(), s.getG() + t.getG(), s.getB() + t.getB());
    }

    static ColorRgb addScaled(const ColorRgb &s, double a, const ColorRgb &t) {
        return ColorRgb(s.getR() + a * t.getR(),
                        s.getG() + a * t.getG(),
                        s.getB() + a * t.getB());
    }

    static ColorRgb subtract(const ColorRgb &s, const ColorRgb &t) {
        return ColorRgb(s.getR() - t.getR(), s.getG() - t.getG(), s.getB() - t.getB());
    }

    static ColorRgb scaledProduct(const ColorRgb &s, double a, const ColorRgb &t) {
        return ColorRgb(s.getR() * a * t.getR(),
                        s.getG() * a * t.getG(),
                        s.getB() * a * t.getB());
    }

    static ColorRgb scaleInverse(double scale, const ColorRgb &color) {
        const double a = scale != 0.0 ? 1.0 / scale : 1.0;
        return ColorRgb(a * color.getR(), a * color.getG(), a * color.getB());
    }

    static double maximumComponent(const ColorRgb &color) {
        const double r = color.getR();
        const double g = color.getG();
        const double b = color.getB();
        return r > g ? (r > b ? r : b) : (g > b ? g : b);
    }

    static double average(const ColorRgb &color) {
        return (color.getR() + color.getG() + color.getB()) / 3.0;
    }

    static ColorRgb maximum(const ColorRgb &s, const ColorRgb &t) {
        const double sr = s.getR(), sg = s.getG(), sb = s.getB();
        const double tr = t.getR(), tg = t.getG(), tb = t.getB();
        return ColorRgb(sr > tr ? sr : tr, sg > tg ? sg : tg, sb > tb ? sb : tb);
    }

    static ColorRgb divide(const ColorRgb &s, const ColorRgb &t) {
        const double sr = s.getR(), sg = s.getG(), sb = s.getB();
        const double tr = t.getR(), tg = t.getG(), tb = t.getB();
        return ColorRgb(tr != 0.0 ? sr / tr : sr,
                        tg != 0.0 ? sg / tg : sg,
                        tb != 0.0 ? sb / tb : sb);
    }
};

#endif
