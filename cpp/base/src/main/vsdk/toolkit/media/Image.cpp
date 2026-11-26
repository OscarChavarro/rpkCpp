#include <cmath>

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/media/Image.h"
#include "vsdk/toolkit/media/RGBPixel.h"
int Image::positiveMod(int value, int modulus) {
    int result = value % modulus;
    if (result < 0) {
        result += modulus;
    }
    return result;
}

char Image::getPixel8bitGrayScale(int x, int y) const {
    RGBPixel* rgb = getPixelRgb(x, y);

    int ur = (int)(unsigned char)rgb->r;
    int ug = (int)(unsigned char)rgb->g;
    int ub = (int)(unsigned char)rgb->b;
    int sum = ur + ug + ub;

    delete rgb;
    return (char)((sum / 3) & 0xFF);
}

ColorRgb* Image::getColorRgbNearest(double x, double y) const {
    double u = x - std::floor(x);
    double v = y - std::floor(y);

    int i = (int)std::floor(u * ((double)(getXSize() - 1)));
    int j = (int)std::floor(v * ((double)(getYSize() - 1)));

    RGBPixel* p = getPixelRgb(i, j);
    int ur = (int)(unsigned char)p->r;
    int ug = (int)(unsigned char)p->g;
    int ub = (int)(unsigned char)p->b;

    ColorRgb* result = new ColorRgb(
        ((double)ur) / 255.0,
        ((double)ug) / 255.0,
        ((double)ub) / 255.0);

    delete p;
    return result;
}

ColorRgb* Image::getColorRgbBiLinear(double x, double y) const {
    double u = x - std::floor(x);
    double v = y - std::floor(y);
    int width = getXSize();
    int height = getYSize();
    double U = u * ((double)width);
    double V = v * ((double)height);
    int i0 = positiveMod((int)std::floor(U), width);
    int j0 = positiveMod((int)std::floor(V), height);
    int i1 = positiveMod(i0 + 1, width);
    int j1 = positiveMod(j0 + 1, height);
    double du = U - (double)i0;
    double dv = V - (double)j0;

    RGBPixel* p = getPixelRgb(i0, j0);
    int ur = (int)(unsigned char)p->r;
    int ug = (int)(unsigned char)p->g;
    int ub = (int)(unsigned char)p->b;
    ColorRgb* F00 = new ColorRgb(
        ((double)ur) / 255.0,
        ((double)ug) / 255.0,
        ((double)ub) / 255.0);
    delete p;

    p = getPixelRgb(i1, j0);
    ur = (int)(unsigned char)p->r;
    ug = (int)(unsigned char)p->g;
    ub = (int)(unsigned char)p->b;
    ColorRgb* F10 = new ColorRgb(
        ((double)ur) / 255.0,
        ((double)ug) / 255.0,
        ((double)ub) / 255.0);
    delete p;

    p = getPixelRgb(i0, j1);
    ur = (int)(unsigned char)p->r;
    ug = (int)(unsigned char)p->g;
    ub = (int)(unsigned char)p->b;
    ColorRgb* F01 = new ColorRgb(
        ((double)ur) / 255.0,
        ((double)ug) / 255.0,
        ((double)ub) / 255.0);
    delete p;

    p = getPixelRgb(i1, j1);
    ur = (int)(unsigned char)p->r;
    ug = (int)(unsigned char)p->g;
    ub = (int)(unsigned char)p->b;
    ColorRgb* F11 = new ColorRgb(
        ((double)ur) / 255.0,
        ((double)ug) / 255.0,
        ((double)ub) / 255.0);
    delete p;

    ColorRgb* FU0 = new ColorRgb(
        F00->r() + du * (F10->r() - F00->r()),
        F00->g() + du * (F10->g() - F00->g()),
        F00->b() + du * (F10->b() - F00->b()));

    ColorRgb* FU1 = new ColorRgb(
        F01->r() + du * (F11->r() - F01->r()),
        F01->g() + du * (F11->g() - F01->g()),
        F01->b() + du * (F11->b() - F01->b()));

    ColorRgb* result = new ColorRgb(
        FU0->r() + dv * (FU1->r() - FU0->r()),
        FU0->g() + dv * (FU1->g() - FU0->g()),
        FU0->b() + dv * (FU1->b() - FU0->b()));

    delete F00;
    delete F10;
    delete F01;
    delete F11;
    delete FU0;
    delete FU1;

    return result;
}

void Image::createTestPattern() {
    int i;
    int j;
    RGBPixel p;

    for (i = 0; i < getXSize(); i++) {
        for (j = 0; j < getYSize(); j++) {
            if (((i % 2 != 0) && (j % 2 == 0)) ||
                ((j % 2 != 0) && (i % 2 == 0))) {
                p.r = (char)255;
                p.g = (char)255;
                p.b = (char)255;
            } else {
                p.r = 0;
                p.g = 0;
                p.b = 0;
            }
            if (j == getYSize() / 2) {
                p.r = (char)255;
                p.g = 0;
                p.b = 0;
            }
            if (i == getXSize() / 2) {
                p.r = 0;
                p.g = (char)255;
                p.b = 0;
            }
            putPixelRgb(i, j, &p);
        }
    }
}
