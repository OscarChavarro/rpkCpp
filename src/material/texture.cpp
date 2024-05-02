#include <cmath>
#include "material/texture.h"

inline void
rgbSetMonochrome(ColorRgb rgb, float val) {
    rgb.set(val, val, val);
}

ColorRgb
evalTextureColor(const TEXTURE *texture, float u, float v) {
    ColorRgb rgb00{};
    ColorRgb rgb10{};
    ColorRgb rgb01{};
    ColorRgb rgb11{};
    ColorRgb rgb{};
    const unsigned char *pix00;
    const unsigned char *pix01;
    const unsigned char *pix10;
    const unsigned char *pix11;
    double u1 = u - std::floor(u);
    double u0 = 1.0 - u1;
    double v1 = v - std::floor(v);
    double v0 = 1.0 - v1;
    int i = (int)(u1 * texture->width);
    int i1 = i + 1;
    int j = (int)(v1 * texture->height);
    int j1 = j + 1;
    if ( i < 0 ) {
        i = 0;
    }
    if ( i >= texture->width ) {
        i = texture->width - 1;
    }
    if ( j < 0 ) {
        j = 0;
    }
    if ( j >= texture->height ) {
        j = texture->height - 1;
    }
    if ( i1 >= texture->width ) {
        i1 -= texture->width;
    }
    if ( j1 >= texture->height ) {
        j1 -= texture->height;
    }

    rgb.clear();
    if ( !texture->data ) {
        return rgb;
    }

    pix00 = texture->data + (j * texture->width + i) * texture->channels;
    pix01 = texture->data + (j1 * texture->width + i) * texture->channels;
    pix10 = texture->data + (j * texture->width + i1) * texture->channels;
    pix11 = texture->data + (j1 * texture->width + i1) * texture->channels;

    switch ( texture->channels ) {
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
