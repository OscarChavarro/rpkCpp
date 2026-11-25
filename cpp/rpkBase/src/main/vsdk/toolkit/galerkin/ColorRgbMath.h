#ifndef __COLOR_RGB_MATH__
#define __COLOR_RGB_MATH__

#include "vsdk/toolkit/common/color/ColorRgb.h"

/**
Operations over contiguous arrays of colors (Galerkin basis coefficients).
Single-color arithmetic lives in ColorRgb.
*/
class ColorRgbMath final {
  public:
    static void arrayCopy(ColorRgb *dest, const ColorRgb *src, int count) {
        for ( int i = 0; i < count; i++ ) {
            dest[i] = src[i];
        }
    }

    static void arrayAdd(ColorRgb *dest, const ColorRgb *src, int count) {
        for ( int i = 0; i < count; i++ ) {
            dest[i] = dest[i].add(src[i]);
        }
    }

    static void arrayClear(ColorRgb *dest, int count) {
        for ( int i = 0; i < count; i++ ) {
            dest[i] = ColorRgb(0.0, 0.0, 0.0);
        }
    }
};

#endif
