#include "material/texture.h"

void PrintTexture(FILE *out, TEXTURE *t) {
    if ( !t ) {
        fprintf(out, "(NULL TEXTURE)");
    } else {
        fprintf(out, "%dx%dx%d texture", t->width, t->height, t->channels);
    }
}

#define RGBSETMONOCHROME(rgb, val) RGBSET(rgb, val, val, val)

COLOR EvalTextureColor(TEXTURE *texture, float u, float v) {
    RGB rgb00, rgb10, rgb01, rgb11, rgb;
    COLOR col;
    unsigned char *pix00, *pix01, *pix10, *pix11;
    double u1 = u - floor(u), u0 = 1. - u1, v1 = v - floor(v), v0 = 1. - v1;
    int i = (u1 * (float) texture->width), i1 = i + 1;
    int j = (v1 * (float) (texture->height)), j1 = j + 1;
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

    COLORCLEAR(col);
    if ( !texture->data ) {
        return col;
    }

    pix00 = texture->data + (j * texture->width + i) * texture->channels;
    pix01 = texture->data + (j1 * texture->width + i) * texture->channels;
    pix10 = texture->data + (j * texture->width + i1) * texture->channels;
    pix11 = texture->data + (j1 * texture->width + i1) * texture->channels;

    switch ( texture->channels ) {
        case 1:
            RGBSETMONOCHROME(rgb00, (float) pix00[0] / 255.);
            RGBSETMONOCHROME(rgb10, (float) pix10[0] / 255.);
            RGBSETMONOCHROME(rgb01, (float) pix01[0] / 255.);
            RGBSETMONOCHROME(rgb11, (float) pix11[0] / 255.);
            break;
        case 3:
        case 4: {
            RGBSET(rgb00, (float) pix00[0] / 255., (float) pix00[1] / 255., (float) pix00[2] / 255.);
            RGBSET(rgb10, (float) pix10[0] / 255., (float) pix10[1] / 255., (float) pix10[2] / 255.);
            RGBSET(rgb01, (float) pix01[0] / 255., (float) pix01[1] / 255., (float) pix01[2] / 255.);
            RGBSET(rgb11, (float) pix11[0] / 255., (float) pix11[1] / 255., (float) pix11[2] / 255.);
        }
            break;
        default:
            break;
    }

    RGBSET(rgb,
           0.25 * (u0 * v0 * rgb00.r + u1 * v0 * rgb10.r + u0 * v1 * rgb01.r + u1 * v1 * rgb11.r),
           0.25 * (u0 * v0 * rgb00.g + u1 * v0 * rgb10.g + u0 * v1 * rgb01.g + u1 * v1 * rgb11.g),
           0.25 * (u0 * v0 * rgb00.b + u1 * v0 * rgb10.b + u0 * v1 * rgb01.b + u1 * v1 * rgb11.b));
    RGBToColor(rgb, &col);
    return col;
}
