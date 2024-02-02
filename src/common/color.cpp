#include "common/color.h"

RGB GLOBAL_material_black = {0.0, 0.0, 0.0};
RGB GLOBAL_material_yellow = {1.0, 1.0, 0.0};
RGB GLOBAL_material_white = {1.0, 1.0, 1.0};

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
