#include "material/color.h"

RGB Black = {0.0, 0.0, 0.0};
RGB Red = {1.0, 0.0, 0.0};
RGB Yellow = {1.0, 1.0, 0.0};
RGB Green = {0.0, 1.0, 0.0};
RGB Blue = {0.0, 0.0, 1.0};
RGB White = {1.0, 1.0, 1.0};

RGB *
ColorToRGB(COLOR col, RGB *rgb) {
    RGBSET(*rgb, col.spec[0], col.spec[1], col.spec[2]);
    return rgb;
}

COLOR *
RGBToColor(RGB rgb, COLOR *col) {
    COLORSET(*col, rgb.r, rgb.g, rgb.b);
    return col;
}
