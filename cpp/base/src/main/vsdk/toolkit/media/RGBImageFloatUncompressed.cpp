#include "vsdk/toolkit/media/RGBImageFloatUncompressed.h"
#include "vsdk/toolkit/media/RGBPixel.h"

RGBImageFloatUncompressed::RGBImageFloatUncompressed() :
    data(nullptr), xSize(0), ySize(0) {
}

RGBImageFloatUncompressed::~RGBImageFloatUncompressed() {
    if (data != nullptr) {
        delete[] data;
        data = nullptr;
    }
}

int RGBImageFloatUncompressed::getSizeInBytes() const {
    return xSize * ySize * COMPONENTS_PER_PIXEL * (int)sizeof(double)
        + 2 * INT_SIZE_IN_BYTES + POINTER_SIZE_IN_BYTES;
}

bool RGBImageFloatUncompressed::init(int width, int height) {
    if (!initNoFill(width, height)) {
        return false;
    }
    for (int i = 0; i < width * height * COMPONENTS_PER_PIXEL; i++) {
        data[i] = 0.0;
    }
    return true;
}

bool RGBImageFloatUncompressed::initNoFill(int width, int height) {
    if (width <= 0 || height <= 0) {
        return false;
    }
    if (data != nullptr && width == xSize && height == ySize) {
        return true;
    }

    try {
        if (data != nullptr) {
            delete[] data;
        }
        data = new double[width * height * COMPONENTS_PER_PIXEL];
    } catch (...) {
        data = nullptr;
        xSize = 0;
        ySize = 0;
        return false;
    }
    xSize = width;
    ySize = height;
    return true;
}

int RGBImageFloatUncompressed::pixelBaseIndex(int x, int y) const {
    return (y * xSize + x) * COMPONENTS_PER_PIXEL;
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
    int index = pixelBaseIndex(x, y);
    data[index] = r;
    data[index + 1] = g;
    data[index + 2] = b;
}

void RGBImageFloatUncompressed::putPixel(int x, int y, const ColorRgb& color) {
    putPixel(x, y, color.getR(), color.getG(), color.getB());
}

ColorRgb RGBImageFloatUncompressed::getPixel(int x, int y) const {
    int index = pixelBaseIndex(x, y);
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
    int index = pixelBaseIndex(x, y);
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
