#include "common/ColorRgb.h"

RGB *
convertColorToRGB(ColorRgb col, RGB *rgb) {
    setRGB(*rgb, col.spectrum[0], col.spectrum[1], col.spectrum[2]);
    return rgb;
}

ColorRgb *
convertRGBToColor(RGB rgb, ColorRgb *col) {
    col->set(rgb.r, rgb.g, rgb.b);
    return col;
}

void
ColorRgb::clear() {
    spectrum[0] = 0;
    spectrum[1] = 0;
    spectrum[2] = 0;
}

void
ColorRgb::set(float v1, float v2, float v3) {
    spectrum[0] = v1;
    spectrum[1] = v2;
    spectrum[2] = v3;
}

void
ColorRgb::setMonochrome(float v) {
    spectrum[0] = v;
    spectrum[1] = v;
    spectrum[2] = v;
}
