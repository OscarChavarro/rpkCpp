#include "common/ColorRgb.h"

RGB *
convertColorToRGB(ColorRgb col, RGB *rgb) {
    setRGB(*rgb, col.spectrum[0], col.spectrum[1], col.spectrum[2]);
    return rgb;
}

ColorRgb *
convertRGBToColor(RGB rgb, ColorRgb *col) {
    colorSet(*col, rgb.r, rgb.g, rgb.b);
    return col;
}
