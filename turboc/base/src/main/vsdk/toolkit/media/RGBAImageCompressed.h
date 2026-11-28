#ifndef __RGBA_IMAGE_COMPRESSED__
#define __RGBA_IMAGE_COMPRESSED__

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/media/Image.h"
class RGBPixel;

/**
Compressed RGBA image storage. Pixel-level access is intentionally unsupported
because the contents remain in GPU-ready compressed blocks.
*/
class RGBAImageCompressed : public Image {

public:
    static const Class CLASS;
    virtual const Class *getClass() const;

    enum {
        COMPRESSION_UNKNOWN = 0,
        COMPRESSION_DXT1 = 1,
        COMPRESSION_DXT3 = 3,
        COMPRESSION_DXT5 = 5
    };

private:
    char* data;
    int xSize;
    int ySize;
    int compressionFormat;
    int compressedDataSize;

public:
    RGBAImageCompressed();
    virtual ~RGBAImageCompressed();

    void detach();

    virtual int getSizeInBytes() const;

    virtual BOOLEAN init(int width, int height);

    virtual BOOLEAN initNoFill(int width, int height);

    BOOLEAN initCompressed(
        int width,
        int height,
        int compressionFormat,
        char* compressedData,
        int dataSize);

    void setRawImage(
        int width,
        int height,
        int compressionFormat,
        char* compressedData,
        int dataSize);

    int getCompressionFormat() const;

    int getCompressedDataSize() const;

    char* getRawImage() const;
    const char* getRawImageDirectBuffer() const;

    virtual void putPixelRgb(int x, int y, RGBPixel* p);

    virtual RGBPixel* getPixelRgb(int x, int y) const;

    virtual void getPixelRgb(int x, int y, RGBPixel* p) const;

    virtual int getXSize() const;

    virtual int getYSize() const;

private:
    static int calculateTopLevelDataSize(int width, int height, int compressionFormat);
    void reportUnsupportedPixelAccess(const char* method) const;
};

#endif
