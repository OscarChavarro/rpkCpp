#include "material/texture.h"

inline void
rgbSetMonochrome(RGB rgb, float val) {
    setRGB(rgb, val, val, val);
}

ColorRgb
evalTextureColor(TEXTURE *texture, float u, float v) {
    RGB rgb00{};
    RGB rgb10{};
    RGB rgb01{};
    RGB rgb11{};
    RGB rgb{};
    ColorRgb col;
    unsigned char *pix00, *pix01, *pix10, *pix11;
    double u1 = u - floor(u), u0 = 1. - u1, v1 = v - floor(v), v0 = 1. - v1;
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

    col.clear();
    if ( !texture->data ) {
        return col;
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
            setRGB(rgb00, (float) pix00[0] / 255.0f, (float) pix00[1] / 255.0f, (float) pix00[2] / 255.0f);
            setRGB(rgb10, (float) pix10[0] / 255.0f, (float) pix10[1] / 255.0f, (float) pix10[2] / 255.0f);
            setRGB(rgb01, (float) pix01[0] / 255.0f, (float) pix01[1] / 255.0f, (float) pix01[2] / 255.0f);
            setRGB(rgb11, (float) pix11[0] / 255.0f, (float) pix11[1] / 255.0f, (float) pix11[2] / 255.0f);
        }
            break;
        default:
            break;
    }

    setRGB(rgb,
           0.25f * (float)(u0 * v0 * rgb00.r + u1 * v0 * rgb10.r + u0 * v1 * rgb01.r + u1 * v1 * rgb11.r),
           0.25f * (float)(u0 * v0 * rgb00.g + u1 * v0 * rgb10.g + u0 * v1 * rgb01.g + u1 * v1 * rgb11.g),
           0.25f * (float)(u0 * v0 * rgb00.b + u1 * v0 * rgb10.b + u0 * v1 * rgb01.b + u1 * v1 * rgb11.b));
    convertRGBToColor(rgb, &col);
    return col;
}
