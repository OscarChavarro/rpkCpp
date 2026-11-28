#include <stddef.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/media/IndexedColorImageUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageUncompressed.h"
#include "vsdk/toolkit/media/RGBAPixel.h"
#include "vsdk/toolkit/media/RGBColorPalette.h"
#include "vsdk/toolkit/media/RGBPixel.h"
IndexedColorImageUncompressed::IndexedColorImageUncompressed(RGBColorPalette* colorTable_) :
    data(NULL), xSize(0), ySize(0), staticColor(NULL) {
    if (colorTable_ != NULL) {
        colorTable = colorTable_;
    } else {
        colorTable = new RGBColorPalette();
    }
}

IndexedColorImageUncompressed::~IndexedColorImageUncompressed() {
    if (data != NULL) {
        delete[] data;
        data = NULL;
    }
    if (colorTable != NULL) {
        delete colorTable;
        colorTable = NULL;
    }
    if (staticColor != NULL) {
        delete staticColor;
        staticColor = NULL;
    }
}

int IndexedColorImageUncompressed::getSizeInBytes() const {
    return xSize * ySize + 3 * INT_SIZE_IN_BYTES + POINTER_SIZE_IN_BYTES;
}

BOOLEAN IndexedColorImageUncompressed::init(int width, int height) {
    try {
        if (data != NULL) {
            delete[] data;
        }
        data = new char[width * height];
        if (data == NULL) {
            // new without exceptions (Turbo C++)
            return FALSE;
        }
        for (int i = 0; i < width * height; i++) {
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

BOOLEAN IndexedColorImageUncompressed::initNoFill(int width, int height) {
    try {
        if (data != NULL) {
            delete[] data;
        }
        data = new char[width * height];
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

int IndexedColorImageUncompressed::pixelBaseIndex(int x, int y) const {
    return xSize * y + x;
}

void IndexedColorImageUncompressed::putPixel(int x, int y, char colorIndex) {
    int index = pixelBaseIndex(x, y);
    if (index >= 0 && index < xSize * ySize) {
        data[index] = colorIndex;
    }
}

void IndexedColorImageUncompressed::putPixelRgb(int x, int y, RGBPixel* p) {
    int ur = (int)(unsigned char)p->r;
    int ug = (int)(unsigned char)p->g;
    int ub = (int)(unsigned char)p->b;

    ColorRgb c(
        ((double)ur) / 255.0,
        ((double)ug) / 255.0,
        ((double)ub) / 255.0);

    int index = colorTable->selectNearestIndexToRgb(c);
    putPixel(x, y, (char)index);
}

char IndexedColorImageUncompressed::getPixel(int x, int y) const {
    int index = pixelBaseIndex(x, y);
    if (index >= 0 && index < xSize * ySize) {
        return data[index];
    }
    return 0;
}

RGBPixel* IndexedColorImageUncompressed::getPixelRgb(int x, int y) const {
    RGBPixel* p = new RGBPixel();
    // The color is changed by this const method (no `mutable` in Turbo C++)
    IndexedColorImageUncompressed *self = (IndexedColorImageUncompressed *)this;
    int index = pixelBaseIndex(x, y);

    double val = ((double)(unsigned char)data[index]) / 255.0;

    if (self->staticColor != NULL) {
        delete self->staticColor;
    }
    self->staticColor = colorTable->evalLinear(val);

    p->r = (char)((int)(self->staticColor->r() * 255.0));
    p->g = (char)((int)(self->staticColor->g() * 255.0));
    p->b = (char)((int)(self->staticColor->b() * 255.0));

    return p;
}

void IndexedColorImageUncompressed::getPixelRgb(int x, int y, RGBPixel* p) const {
    // The color is changed by this const method (no `mutable` in Turbo C++)
    IndexedColorImageUncompressed *self = (IndexedColorImageUncompressed *)this;
    int index = pixelBaseIndex(x, y);

    double val = ((double)(unsigned char)data[index]) / 255.0;

    if (self->staticColor != NULL) {
        delete self->staticColor;
    }
    self->staticColor = colorTable->evalLinear(val);

    p->r = (char)((int)(self->staticColor->r() * 255.0));
    p->g = (char)((int)(self->staticColor->g() * 255.0));
    p->b = (char)((int)(self->staticColor->b() * 255.0));
}

int IndexedColorImageUncompressed::getXSize() const {
    return xSize;
}

int IndexedColorImageUncompressed::getYSize() const {
    return ySize;
}

RGBColorPalette* IndexedColorImageUncompressed::getColorTable() const {
    return colorTable;
}

void IndexedColorImageUncompressed::setColorTable(RGBColorPalette* colorTable_) {
    colorTable = colorTable_;
}

char* IndexedColorImageUncompressed::getRawImage() const {
    if (data == NULL) {
        return NULL;
    }
    char* copy = new char[xSize * ySize];
    for (int i = 0; i < xSize * ySize; i++) {
        copy[i] = data[i];
    }
    return copy;
}

void IndexedColorImageUncompressed::setRawImage(int width, int height, char* imageData) {
    xSize = width;
    ySize = height;

    if (data != NULL) {
        delete[] data;
    }
    if (imageData != NULL) {
        int size = width * height;
        data = new char[size];
        for (int i = 0; i < size; i++) {
            data[i] = imageData[i];
        }
    } else {
        data = NULL;
    }
}

IndexedColorImageUncompressed* IndexedColorImageUncompressed::clone() const {
    IndexedColorImageUncompressed* copy = new IndexedColorImageUncompressed(colorTable);
    int xxSize = getXSize();
    int yySize = getYSize();
    copy->init(xxSize, yySize);

    for (int x = 0; x < xxSize; x++) {
        for (int y = 0; y < yySize; y++) {
            copy->putPixel(x, y, getPixel(x, y));
        }
    }
    return copy;
}

RGBAImageUncompressed* IndexedColorImageUncompressed::exportToRgbaImage() const
{
    RGBAImageUncompressed* copy;
    int xxSize = getXSize();
    int yySize = getYSize();
    int x;
    int y;
    char source;
    RGBAPixel target;

    copy = new RGBAImageUncompressed();
    copy->init(xxSize, yySize);
    target.a = (char)128;
    for ( x = 0; x < xxSize; x++ ) {
        for ( y = 0; y < yySize; y++ ) {
            source = getPixel(x, y);
            target.r = target.g = target.b = source;
            copy->putPixel(x, y, &target);
        }
    }

    return copy;
}

const Class IndexedColorImageUncompressed::CLASS = { "IndexedColorImageUncompressed", &Image::CLASS };

const Class *
IndexedColorImageUncompressed::getClass() const
{
    return &CLASS;
}
