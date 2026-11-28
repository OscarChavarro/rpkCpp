#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/media/IndexedColorImageUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageUncompressed.h"
#include "vsdk/toolkit/media/RGBAPixel.h"
#include "vsdk/toolkit/media/RGBColorPalette.h"
#include "vsdk/toolkit/media/RGBPixel.h"
IndexedColorImageUncompressed::IndexedColorImageUncompressed(RGBColorPalette* colorTable_) :
    data(nullptr), xSize(0), ySize(0), staticColor(nullptr) {
    if (colorTable_ != nullptr) {
        colorTable = colorTable_;
    } else {
        colorTable = new RGBColorPalette();
    }
}

IndexedColorImageUncompressed::~IndexedColorImageUncompressed() {
    if (data != nullptr) {
        delete[] data;
        data = nullptr;
    }
    if (colorTable != nullptr) {
        delete colorTable;
        colorTable = nullptr;
    }
    if (staticColor != nullptr) {
        delete staticColor;
        staticColor = nullptr;
    }
}

int IndexedColorImageUncompressed::getSizeInBytes() const {
    return xSize * ySize + 3 * INT_SIZE_IN_BYTES + POINTER_SIZE_IN_BYTES;
}

bool IndexedColorImageUncompressed::init(int width, int height) {
    try {
        if (data != nullptr) {
            delete[] data;
        }
        data = new char[width * height];
        for (int i = 0; i < width * height; i++) {
            data[i] = 0;
        }
    } catch (...) {
        data = nullptr;
        return false;
    }
    xSize = width;
    ySize = height;
    return true;
}

bool IndexedColorImageUncompressed::initNoFill(int width, int height) {
    try {
        if (data != nullptr) {
            delete[] data;
        }
        data = new char[width * height];
    } catch (...) {
        data = nullptr;
        return false;
    }
    xSize = width;
    ySize = height;
    return true;
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
    int index = pixelBaseIndex(x, y);

    double val = ((double)(unsigned char)data[index]) / 255.0;

    if (staticColor != nullptr) {
        delete staticColor;
    }
    staticColor = colorTable->evalLinear(val);

    p->r = (char)((int)(staticColor->r() * 255.0));
    p->g = (char)((int)(staticColor->g() * 255.0));
    p->b = (char)((int)(staticColor->b() * 255.0));

    return p;
}

void IndexedColorImageUncompressed::getPixelRgb(int x, int y, RGBPixel* p) const {
    int index = pixelBaseIndex(x, y);

    double val = ((double)(unsigned char)data[index]) / 255.0;

    if (staticColor != nullptr) {
        delete staticColor;
    }
    staticColor = colorTable->evalLinear(val);

    p->r = (char)((int)(staticColor->r() * 255.0));
    p->g = (char)((int)(staticColor->g() * 255.0));
    p->b = (char)((int)(staticColor->b() * 255.0));
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
    if (data == nullptr) {
        return nullptr;
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

    if (data != nullptr) {
        delete[] data;
    }
    if (imageData != nullptr) {
        int size = width * height;
        data = new char[size];
        for (int i = 0; i < size; i++) {
            data[i] = imageData[i];
        }
    } else {
        data = nullptr;
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
