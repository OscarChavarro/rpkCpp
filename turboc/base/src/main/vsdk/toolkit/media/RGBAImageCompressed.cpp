#include <stdio.h>
#include <string.h>

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/media/RGBAImageCompressed.h"
#include "vsdk/toolkit/media/RGBPixel.h"
RGBAImageCompressed::RGBAImageCompressed() :
    data(NULL), xSize(0), ySize(0),
    compressionFormat(COMPRESSION_UNKNOWN), compressedDataSize(0) {
}

RGBAImageCompressed::~RGBAImageCompressed() {
    if (data != NULL) {
        delete[] data;
        data = NULL;
    }
}

void RGBAImageCompressed::detach() {
    if (data != NULL) {
        delete[] data;
        data = NULL;
    }
}

int RGBAImageCompressed::getSizeInBytes() const {
    int dataSize = (data == NULL) ? 0 : compressedDataSize;
    return dataSize + 4 * INT_SIZE_IN_BYTES + POINTER_SIZE_IN_BYTES;
}

BOOLEAN RGBAImageCompressed::init(int width, int height) {
    xSize = width;
    ySize = height;
    compressionFormat = COMPRESSION_UNKNOWN;
    compressedDataSize = 0;
    if (data != NULL) {
        delete[] data;
    }
    data = new char[0];
    return TRUE;
}

BOOLEAN RGBAImageCompressed::initNoFill(int width, int height) {
    return init(width, height);
}

int RGBAImageCompressed::calculateTopLevelDataSize(
    int width,
    int height,
    int compressionFormat) {
    int blockSize;

    if (compressionFormat == COMPRESSION_DXT1) {
        blockSize = 8;
    } else if (compressionFormat == COMPRESSION_DXT3 ||
               compressionFormat == COMPRESSION_DXT5) {
        blockSize = 16;
    } else {
        return 0;
    }

    int blockWidth = Math::max(1, (width + 3) / 4);
    int blockHeight = Math::max(1, (height + 3) / 4);
    return blockWidth * blockHeight * blockSize;
}

BOOLEAN RGBAImageCompressed::initCompressed(
    int width,
    int height,
    int compressionFormat_,
    char* compressedData,
    int dataSize) {
    if (width <= 0 || height <= 0 || compressedData == NULL) {
        data = NULL;
        xSize = 0;
        ySize = 0;
        compressionFormat = COMPRESSION_UNKNOWN;
        compressedDataSize = 0;
        return FALSE;
    }

    xSize = width;
    ySize = height;
    compressionFormat = compressionFormat_;
    compressedDataSize = calculateTopLevelDataSize(width, height, compressionFormat_);
    if (dataSize < compressedDataSize) {
        data = NULL;
        xSize = 0;
        ySize = 0;
        compressionFormat = COMPRESSION_UNKNOWN;
        compressedDataSize = 0;
        return FALSE;
    }

    if (data != NULL) {
        delete[] data;
    }
    data = new char[dataSize];
    memcpy(data, compressedData, dataSize);
    return TRUE;
}

void RGBAImageCompressed::setRawImage(
    int width,
    int height,
    int compressionFormat_,
    char* compressedData,
    int dataSize) {
    initCompressed(width, height, compressionFormat_, compressedData, dataSize);
}

int RGBAImageCompressed::getCompressionFormat() const {
    return compressionFormat;
}

int RGBAImageCompressed::getCompressedDataSize() const {
    return compressedDataSize;
}

char* RGBAImageCompressed::getRawImage() const {
    if (data == NULL) {
        return NULL;
    }
    char* copy = new char[compressedDataSize];
    memcpy(copy, data, compressedDataSize);
    return copy;
}

const char* RGBAImageCompressed::getRawImageDirectBuffer() const {
    return data;
}

void RGBAImageCompressed::reportUnsupportedPixelAccess(const char* method) const {
    fprintf(stderr, "FATAL_ERROR in %s: Pixel access is not implemented for RGBAImageCompressed.\n", method);
}

void RGBAImageCompressed::putPixelRgb(int /*x*/, int /*y*/, RGBPixel* /*p*/) {
    reportUnsupportedPixelAccess("putPixelRgb");
}

RGBPixel* RGBAImageCompressed::getPixelRgb(int /*x*/, int /*y*/) const {
    reportUnsupportedPixelAccess("getPixelRgb");
    return new RGBPixel();
}

void RGBAImageCompressed::getPixelRgb(int /*x*/, int /*y*/, RGBPixel* /*p*/) const {
    reportUnsupportedPixelAccess("getPixelRgb");
}

int RGBAImageCompressed::getXSize() const {
    return xSize;
}

int RGBAImageCompressed::getYSize() const {
    return ySize;
}

const Class RGBAImageCompressed::CLASS = { "RGBAImageCompressed", &Image::CLASS };

const Class *
RGBAImageCompressed::getClass() const
{
    return &CLASS;
}
