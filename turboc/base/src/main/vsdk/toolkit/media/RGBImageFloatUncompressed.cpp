#include <stddef.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/media/RGBImageFloatUncompressed.h"
#include "vsdk/toolkit/media/RGBPixel.h"

RGBImageFloatUncompressed::RGBImageFloatUncompressed() :
    data(NULL), xSize(0), ySize(0) {
}

RGBImageFloatUncompressed::~RGBImageFloatUncompressed() {
    if (data != NULL) {
        delete[] data;
        data = NULL;
    }
}

int RGBImageFloatUncompressed::getSizeInBytes() const {
    return xSize * ySize * COMPONENTS_PER_PIXEL * (int)sizeof(double)
        + 2 * INT_SIZE_IN_BYTES + POINTER_SIZE_IN_BYTES;
}

BOOLEAN RGBImageFloatUncompressed::init(int width, int height) {
    if (!initNoFill(width, height)) {
        return FALSE;
    }
    long n = (long)width * (long)height * (long)COMPONENTS_PER_PIXEL;
    long i;
    for (i = 0; i < n; i++) {
        data[i] = 0.0;
    }
    return TRUE;
}

BOOLEAN RGBImageFloatUncompressed::initNoFill(int width, int height) {
    if (width <= 0 || height <= 0) {
        return FALSE;
    }
    if (data != NULL && width == xSize && height == ySize) {
        return TRUE;
    }

    long n = (long)width * (long)height * (long)COMPONENTS_PER_PIXEL;
#ifdef __TURBOC__
    // One segment at most: no huge pointers in the program memory
    if (n * (long)sizeof(double) > 65535L) {
        return FALSE;
    }
#endif

    try {
        if (data != NULL) {
            delete[] data;
            data = NULL;
        }
        data = new double[(size_t)n];
        if (data == NULL) {
            // new without exceptions (Turbo C++)
            xSize = 0;
            ySize = 0;
            return FALSE;
        }
    }
#ifndef VSDK_WITHOUT_EXCEPTIONS
    catch (...) {
        data = NULL;
        xSize = 0;
        ySize = 0;
        return FALSE;
    }
#endif
    xSize = width;
    ySize = height;
    return TRUE;
}

long RGBImageFloatUncompressed::pixelBaseIndex(int x, int y) const {
    return ((long)y * (long)xSize + (long)x) * (long)COMPONENTS_PER_PIXEL;
}

char RGBImageFloatUncompressed::componentToByte(double value) {
    if (!(value > 0.0)) {
        return 0;
    }
    if (value >= 1.0) {
        return (char)255;
    }
    return (char)(unsigned char)(value * 255.0);
}

void RGBImageFloatUncompressed::putPixel(int x, int y, double r, double g, double b) {
    long index = pixelBaseIndex(x, y);
    data[index] = r;
    data[index + 1] = g;
    data[index + 2] = b;
}

void RGBImageFloatUncompressed::putPixel(int x, int y, const ColorRgb& color) {
    putPixel(x, y, color.getR(), color.getG(), color.getB());
}

ColorRgb RGBImageFloatUncompressed::getPixel(int x, int y) const {
    long index = pixelBaseIndex(x, y);
    return ColorRgb(data[index], data[index + 1], data[index + 2]);
}

void RGBImageFloatUncompressed::putPixelRgb(int x, int y, RGBPixel* p) {
    putPixel(
        x,
        y,
        (double)(unsigned char)p->r / 255.0,
        (double)(unsigned char)p->g / 255.0,
        (double)(unsigned char)p->b / 255.0);
}

RGBPixel* RGBImageFloatUncompressed::getPixelRgb(int x, int y) const {
    RGBPixel* p = new RGBPixel();
    getPixelRgb(x, y, p);
    return p;
}

void RGBImageFloatUncompressed::getPixelRgb(int x, int y, RGBPixel* p) const {
    long index = pixelBaseIndex(x, y);
    p->r = componentToByte(data[index]);
    p->g = componentToByte(data[index + 1]);
    p->b = componentToByte(data[index + 2]);
}

int RGBImageFloatUncompressed::getXSize() const {
    return xSize;
}

int RGBImageFloatUncompressed::getYSize() const {
    return ySize;
}

void RGBImageFloatUncompressed::dispose() {
    Entity::dispose();
}

const Class RGBImageFloatUncompressed::CLASS = { "RGBImageFloatUncompressed", &Image::CLASS };

const Class *
RGBImageFloatUncompressed::getClass() const
{
    return &CLASS;
}
