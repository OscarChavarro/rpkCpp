#include <stddef.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/media/RGBAImageUncompressed.h"
#include "vsdk/toolkit/media/RGBAPixel.h"
#include "vsdk/toolkit/media/RGBPixel.h"
RGBAImageUncompressed::RGBAImageUncompressed() :
    data(NULL), xSize(0), ySize(0) {
}

RGBAImageUncompressed::~RGBAImageUncompressed() {
    if (data != NULL) {
        delete[] data;
        data = NULL;
    }
}

void RGBAImageUncompressed::detach() {
    if (data != NULL) {
        delete[] data;
        data = NULL;
    }
}

int RGBAImageUncompressed::getSizeInBytes() const {
    return xSize * ySize * 4 + 2 * INT_SIZE_IN_BYTES + POINTER_SIZE_IN_BYTES;
}

BOOLEAN RGBAImageUncompressed::init(int width, int height) {
    try {
        if (data != NULL) {
            delete[] data;
        }
        data = new char[width * height * BYTES_PER_PIXEL];
        if (data == NULL) {
            // new without exceptions (Turbo C++)
            return FALSE;
        }
        for (int i = 0; i < width * height * BYTES_PER_PIXEL; i++) {
            data[i] = 0;
        }
    }
#ifndef VSDK_WITHOUT_EXCEPTIONS
    catch (...) {
        data = NULL;
        return FALSE;
    }
#endif
    xSize = width;
    ySize = height;
    return TRUE;
}

BOOLEAN RGBAImageUncompressed::initNoFill(int width, int height) {
    if (data != NULL && width == xSize && height == ySize) {
        return TRUE;
    }

    try {
        if (data != NULL) {
            delete[] data;
        }
        data = new char[width * height * BYTES_PER_PIXEL];
        if (data == NULL) {
            // new without exceptions (Turbo C++)
            return FALSE;
        }
    }
#ifndef VSDK_WITHOUT_EXCEPTIONS
    catch (...) {
        data = NULL;
        return FALSE;
    }
#endif
    xSize = width;
    ySize = height;
    return TRUE;
}

int RGBAImageUncompressed::pixelBaseIndex(int x, int y) const {
    return (ySize - 1 - y) * xSize * BYTES_PER_PIXEL + x * BYTES_PER_PIXEL;
}

void RGBAImageUncompressed::putPixel(int x, int y, char r, char g, char b, char a) {
    int index = pixelBaseIndex(x, y);
    data[index] = r;
    data[index + 1] = g;
    data[index + 2] = b;
    data[index + 3] = a;
}

void RGBAImageUncompressed::putPixelA(int x, int y, char r, char g, char b, char a) {
    putPixel(x, y, r, g, b, a);
}

void RGBAImageUncompressed::putPixel(int x, int y, const RGBAPixel* p) {
    int index = pixelBaseIndex(x, y);
    data[index] = p->r;
    data[index + 1] = p->g;
    data[index + 2] = p->b;
    data[index + 3] = p->a;
}

void RGBAImageUncompressed::putPixelRgb(int x, int y, RGBPixel* p) {
    int index = pixelBaseIndex(x, y);
    data[index] = p->r;
    data[index + 1] = p->g;
    data[index + 2] = p->b;
    data[index + 3] = (char)255;
}

RGBAPixel* RGBAImageUncompressed::getPixel(int x, int y) const {
    RGBAPixel* p = new RGBAPixel();
    int index = pixelBaseIndex(x, y);
    p->r = data[index];
    p->g = data[index + 1];
    p->b = data[index + 2];
    p->a = data[index + 3];
    return p;
}

RGBPixel* RGBAImageUncompressed::getPixelRgb(int x, int y) const {
    RGBPixel* p = new RGBPixel();
    int index = pixelBaseIndex(x, y);
    p->r = data[index];
    p->g = data[index + 1];
    p->b = data[index + 2];
    return p;
}

RGBAPixel* RGBAImageUncompressed::getPixelRgba(int x, int y) const {
    return getPixel(x, y);
}

void RGBAImageUncompressed::getPixelRgb(int x, int y, RGBPixel* p) const {
    int index = pixelBaseIndex(x, y);
    p->r = data[index];
    p->g = data[index + 1];
    p->b = data[index + 2];
}

void RGBAImageUncompressed::getPixelRgba(int x, int y, RGBAPixel* p) const {
    int index = pixelBaseIndex(x, y);
    p->r = data[index];
    p->g = data[index + 1];
    p->b = data[index + 2];
    p->a = data[index + 3];
}

int RGBAImageUncompressed::getXSize() const {
    return xSize;
}

int RGBAImageUncompressed::getYSize() const {
    return ySize;
}

char* RGBAImageUncompressed::getRawImage() const {
    if (data == NULL) {
        return NULL;
    }
    char* copy = new char[xSize * ySize * BYTES_PER_PIXEL];
    for (int i = 0; i < xSize * ySize * BYTES_PER_PIXEL; i++) {
        copy[i] = data[i];
    }
    return copy;
}

void RGBAImageUncompressed::setRawImage(int width, int height, char* imageData) {
    xSize = width;
    ySize = height;

    if (data != NULL) {
        delete[] data;
    }
    if (imageData != NULL) {
        int size = width * height * BYTES_PER_PIXEL;
        data = new char[size];
        for (int i = 0; i < size; i++) {
            data[i] = imageData[i];
        }
    } else {
        data = NULL;
    }
}

RGBAImageUncompressed* RGBAImageUncompressed::clone() const {
    RGBAImageUncompressed* copy = new RGBAImageUncompressed();
    int xxSize = getXSize();
    int yySize = getYSize();
    copy->init(xxSize, yySize);

    for (int x = 0; x < xxSize; x++) {
        for (int y = 0; y < yySize; y++) {
            RGBAPixel* pixel = getPixel(x, y);
            copy->putPixel(x, y, pixel);
            delete pixel;
        }
    }
    return copy;
}

void RGBAImageUncompressed::dispose() {
    Entity::dispose();
}

const Class RGBAImageUncompressed::CLASS = { "RGBAImageUncompressed", &Image::CLASS };

const Class *
RGBAImageUncompressed::getClass() const
{
    return &CLASS;
}
