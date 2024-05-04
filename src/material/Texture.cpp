#include <cmath>
#include "material/Texture.h"

inline void
rgbSetMonochrome(ColorRgb rgb, float val) {
    rgb.set(val, val, val);
}

ColorRgb
Texture::evaluateColor(float u, float v) const {
    double u1 = u - std::floor(u);
    double u0 = 1.0 - u1;
    double v1 = v - std::floor(v);
    double v0 = 1.0 - v1;
    int i = (int)(u1 * width);
    int i1 = i + 1;
    int j = (int)(v1 * height);
    int j1 = j + 1;
    if ( i < 0 ) {
        i = 0;
    }
    if ( i >= width ) {
        i = width - 1;
    }
    if ( j < 0 ) {
        j = 0;
    }
    if ( j >= height ) {
        j = height - 1;
    }
    if ( i1 >= width ) {
        i1 -= width;
    }
    if ( j1 >= height ) {
        j1 -= height;
    }

    ColorRgb rgb{};

    rgb.clear();
    if ( !data ) {
        return rgb;
    }

    const unsigned char *pix00 = data + (j * width + i) * channels;
    const unsigned char *pix01 = data + (j1 * width + i) * channels;
    const unsigned char *pix10 = data + (j * width + i1) * channels;
    const unsigned char *pix11 = data + (j1 * width + i1) * channels;

    ColorRgb rgb00{};
    ColorRgb rgb10{};
    ColorRgb rgb01{};
    ColorRgb rgb11{};

    switch ( channels ) {
        case 1:
            rgbSetMonochrome(rgb00, (float) pix00[0] / 255.0f);
            rgbSetMonochrome(rgb10, (float) pix10[0] / 255.0f);
            rgbSetMonochrome(rgb01, (float) pix01[0] / 255.0f);
            rgbSetMonochrome(rgb11, (float) pix11[0] / 255.0f);
            break;
        case 3:
        case 4: {
            rgb00.set((float) pix00[0] / 255.0f, (float) pix00[1] / 255.0f, (float) pix00[2] / 255.0f);
            rgb10.set((float) pix10[0] / 255.0f, (float) pix10[1] / 255.0f, (float) pix10[2] / 255.0f);
            rgb01.set((float) pix01[0] / 255.0f, (float) pix01[1] / 255.0f, (float) pix01[2] / 255.0f);
            rgb11.set((float) pix11[0] / 255.0f, (float) pix11[1] / 255.0f, (float) pix11[2] / 255.0f);
        }
            break;
        default:
            break;
    }

    rgb.set(
        0.25f * (float) (u0 * v0 * rgb00.r + u1 * v0 * rgb10.r + u0 * v1 * rgb01.r + u1 * v1 * rgb11.r),
        0.25f * (float) (u0 * v0 * rgb00.g + u1 * v0 * rgb10.g + u0 * v1 * rgb01.g + u1 * v1 * rgb11.g),
        0.25f * (float) (u0 * v0 * rgb00.b + u1 * v0 * rgb10.b + u0 * v1 * rgb01.b + u1 * v1 * rgb11.b));
    return rgb;
}
