#include "common/color.h"

RGB *
convertColorToRGB(COLOR col, RGB *rgb) {
    setRGB(*rgb, col.r, col.g, col.b);
    return rgb;
}

COLOR *
convertRGBToColor(RGB rgb, COLOR *col) {
    colorSet(*col, rgb.r, rgb.g, rgb.b);
    return col;
}
