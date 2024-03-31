#include "common/color.h"

RGB *
convertColorToRGB(COLOR col, RGB *rgb) {
    setRGB(*rgb, col.spec[0], col.spec[1], col.spec[2]);
    return rgb;
}

COLOR *
convertRGBToColor(RGB rgb, COLOR *col) {
    colorSet(*col, rgb.r, rgb.g, rgb.b);
    return col;
}
