#ifndef __RGBA_IMAGE_UNCOMPRESSED__
#define __RGBA_IMAGE_UNCOMPRESSED__

#include "vsdk/toolkit/media/Image.h"
class RGBAPixel;

/**
Current class is a specific low level implementation of an uncompressed
32 bits per pixel RGBA image over a byte array (ordered in a sequential array
of RGBA bytes, row by row from upper left pixel, and left to right on each
row).
*/
class RGBAImageUncompressed : public Image {

private:
    static const int BYTES_PER_PIXEL = 4;

    char* data;
    int xSize;
    int ySize;

    int pixelBaseIndex(int x, int y) const;

public:
    RGBAImageUncompressed();
    virtual ~RGBAImageUncompressed();

    void detach();

    virtual int getSizeInBytes() const override;

    virtual bool init(int width, int height) override;

    virtual bool initNoFill(int width, int height) override;

    void putPixel(int x, int y, char r, char g, char b, char a);
    void putPixelA(int x, int y, char r, char g, char b, char a);
    void putPixel(int x, int y, const RGBAPixel* p);

    virtual void putPixelRgb(int x, int y, RGBPixel* p) override;

    RGBAPixel* getPixel(int x, int y) const;

    virtual RGBPixel* getPixelRgb(int x, int y) const override;

    RGBAPixel* getPixelRgba(int x, int y) const;

    virtual void getPixelRgb(int x, int y, RGBPixel* p) const override;

    void getPixelRgba(int x, int y, RGBAPixel* p) const;

    virtual int getXSize() const override;

    virtual int getYSize() const override;

    char* getRawImage() const;

    void setRawImage(int xSize, int ySize, char* data);

    RGBAImageUncompressed* clone() const;

    virtual void dispose() override;
};

#endif
