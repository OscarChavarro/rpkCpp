#include <cstdio>
#include <cstring>

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/media/RGBAImageCompressed.h"
#include "vsdk/toolkit/media/RGBPixel.h"
RGBAImageCompressed::RGBAImageCompressed() :
    data(nullptr), xSize(0), ySize(0),
    compressionFormat(COMPRESSION_UNKNOWN), compressedDataSize(0) {
}

RGBAImageCompressed::~RGBAImageCompressed() {
    if (data != nullptr) {
        delete[] data;
        data = nullptr;
    }
}

void RGBAImageCompressed::detach() {
    if (data != nullptr) {
        delete[] data;
        data = nullptr;
    }
}

int RGBAImageCompressed::getSizeInBytes() const {
    int dataSize = (data == nullptr) ? 0 : compressedDataSize;
    return dataSize + 4 * INT_SIZE_IN_BYTES + POINTER_SIZE_IN_BYTES;
}

bool RGBAImageCompressed::init(int width, int height) {
    xSize = width;
    ySize = height;
    compressionFormat = COMPRESSION_UNKNOWN;
    compressedDataSize = 0;
    if (data != nullptr) {
        delete[] data;
    }
    data = new char[0];
    return true;
}

bool RGBAImageCompressed::initNoFill(int width, int height) {
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

    int blockWidth = java::Math::max(1, (width + 3) / 4);
    int blockHeight = java::Math::max(1, (height + 3) / 4);
    return blockWidth * blockHeight * blockSize;
}

bool RGBAImageCompressed::initCompressed(
    int width,
    int height,
    int compressionFormat_,
    char* compressedData,
    int dataSize) {
    if (width <= 0 || height <= 0 || compressedData == nullptr) {
        data = nullptr;
        xSize = 0;
        ySize = 0;
        compressionFormat = COMPRESSION_UNKNOWN;
        compressedDataSize = 0;
        return false;
    }

    xSize = width;
    ySize = height;
    compressionFormat = compressionFormat_;
    compressedDataSize = calculateTopLevelDataSize(width, height, compressionFormat_);
    if (dataSize < compressedDataSize) {
        data = nullptr;
        xSize = 0;
        ySize = 0;
        compressionFormat = COMPRESSION_UNKNOWN;
        compressedDataSize = 0;
        return false;
    }

    if (data != nullptr) {
        delete[] data;
    }
    data = new char[dataSize];
    std::memcpy(data, compressedData, dataSize);
    return true;
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
    if (data == nullptr) {
        return nullptr;
    }
    char* copy = new char[compressedDataSize];
    std::memcpy(copy, data, compressedDataSize);
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
