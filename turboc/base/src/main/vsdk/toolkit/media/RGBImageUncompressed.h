#ifndef __RGB_IMAGE_UNCOMPRESSED__
#define __RGB_IMAGE_UNCOMPRESSED__

#if defined(__TURBOC__) && !defined(TURBO_C_286_ENABLED) && !defined(TURBO_C_8086_REAL_MODE_ENABLED)
#error Select TURBO_C_286_ENABLED or TURBO_C_8086_REAL_MODE_ENABLED
#endif
#if defined(TURBO_C_286_ENABLED) && defined(TURBO_C_8086_REAL_MODE_ENABLED)
#error The protected 286 and real-mode 8086 backends are mutually exclusive
#endif

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/media/Image.h"
class RGBPixel;
class RGBAImageUncompressed;
#if defined(TURBO_C_286_ENABLED)
class Protected286ModeRgbImage;
#elif defined(TURBO_C_8086_REAL_MODE_ENABLED)
class XmsRgbImage;
#endif

/**
Current class is a specific low level implementation of an uncompressed
24 bits per pixel RGB image over a byte array (ordered in a sequential array
of RGB bytes, row by row from upper left pixel, and left to right on each
row).
*/
class RGBImageUncompressed : public Image {

private:
    enum { BYTES_PER_PIXEL = 3 };

    char* data;
    int xSize;
    int ySize;
    int rowStride;
#if defined(TURBO_C_286_ENABLED)
    Protected286ModeRgbImage* dosImage;
#elif defined(TURBO_C_8086_REAL_MODE_ENABLED)
    XmsRgbImage* dosImage;
#endif

    int pixelBaseIndex(int x, int y) const;

public:
    static const Class CLASS;
    virtual const Class *getClass() const;

    RGBImageUncompressed();
    virtual ~RGBImageUncompressed();

    void detach();

    virtual int getSizeInBytes() const;

    virtual BOOLEAN init(int width, int height);

    virtual BOOLEAN initNoFill(int width, int height);

    void putPixel(int x, int y, char r, char g, char b);
    void putPixel(int x, int y, const RGBPixel* p);

    virtual void putPixelRgb(int x, int y, RGBPixel* p);

    RGBPixel* getPixel(int x, int y) const;

    virtual RGBPixel* getPixelRgb(int x, int y) const;

    virtual void getPixelRgb(int x, int y, RGBPixel* p) const;

    virtual int getXSize() const;

    virtual int getYSize() const;

    char* getRawImage() const;

    // The pixels themselves (no copy), in the order of `getRawImage`: to read
    // or modify the image without copying it. Null if not initialized, or if
    // the pixels are kept out of the program memory (DOS images).
    char* getRawImageDirectBuffer();

    void setRawImage(int xSize, int ySize, char* data);

    RGBImageUncompressed* clone() const;

    RGBAImageUncompressed* cloneToRgba() const;

    virtual void dispose();
};

#endif
