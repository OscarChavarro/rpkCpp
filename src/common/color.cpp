#include "common/color.h"

RGB *
convertColorToRGB(COLOR col, RGB *rgb) {
    setRGB(*rgb, col.spectrum[0], col.spectrum[1], col.spectrum[2]);
    return rgb;
}

COLOR *
convertRGBToColor(RGB rgb, COLOR *col) {
    colorSet(*col, rgb.r, rgb.g, rgb.b);
    return col;
}
