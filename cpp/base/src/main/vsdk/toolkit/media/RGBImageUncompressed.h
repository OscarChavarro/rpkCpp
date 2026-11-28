#ifndef __RGB_IMAGE_UNCOMPRESSED__
#define __RGB_IMAGE_UNCOMPRESSED__

#include "vsdk/toolkit/media/Image.h"
class RGBPixel;
class RGBAImageUncompressed;

/**
Current class is a specific low level implementation of an uncompressed
24 bits per pixel RGB image over a byte array (ordered in a sequential array
of RGB bytes, row by row from upper left pixel, and left to right on each
row).
*/
class RGBImageUncompressed : public Image {

private:
    static const int BYTES_PER_PIXEL = 3;

    char* data;
    int xSize;
    int ySize;
    int rowStride;

    int pixelBaseIndex(int x, int y) const;

public:
    RGBImageUncompressed();
    virtual ~RGBImageUncompressed();

    void detach();

    virtual int getSizeInBytes() const override;

    virtual bool init(int width, int height) override;

    virtual bool initNoFill(int width, int height) override;

    void putPixel(int x, int y, char r, char g, char b);
    void putPixel(int x, int y, const RGBPixel* p);

    virtual void putPixelRgb(int x, int y, RGBPixel* p) override;

    RGBPixel* getPixel(int x, int y) const;

    virtual RGBPixel* getPixelRgb(int x, int y) const override;

    virtual void getPixelRgb(int x, int y, RGBPixel* p) const override;

    virtual int getXSize() const override;

    virtual int getYSize() const override;

    char* getRawImage() const;

    // The pixels themselves (no copy), in the order of `getRawImage`: to read
    // or modify the image without copying it. Null if not initialized.
    char* getRawImageDirectBuffer();

    void setRawImage(int xSize, int ySize, char* data);

    RGBImageUncompressed* clone() const;

    RGBAImageUncompressed* cloneToRgba() const;

    virtual void dispose() override;
};

#endif
