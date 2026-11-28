#ifndef __RGBA_IMAGE_COMPRESSED__
#define __RGBA_IMAGE_COMPRESSED__

#include "vsdk/toolkit/media/Image.h"
class RGBPixel;

/**
Compressed RGBA image storage. Pixel-level access is intentionally unsupported
because the contents remain in GPU-ready compressed blocks.
*/
class RGBAImageCompressed : public Image {

public:
    static const int COMPRESSION_UNKNOWN = 0;
    static const int COMPRESSION_DXT1 = 1;
    static const int COMPRESSION_DXT3 = 3;
    static const int COMPRESSION_DXT5 = 5;

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

    virtual int getSizeInBytes() const override;

    virtual bool init(int width, int height) override;

    virtual bool initNoFill(int width, int height) override;

    bool initCompressed(
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

    virtual void putPixelRgb(int x, int y, RGBPixel* p) override;

    virtual RGBPixel* getPixelRgb(int x, int y) const override;

    virtual void getPixelRgb(int x, int y, RGBPixel* p) const override;

    virtual int getXSize() const override;

    virtual int getYSize() const override;

private:
    static int calculateTopLevelDataSize(int width, int height, int compressionFormat);
    void reportUnsupportedPixelAccess(const char* method) const;
};

#endif
