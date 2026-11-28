#ifndef __RGB_IMG_FLOAT_UNCOMPRESSED__
#define __RGB_IMG_FLOAT_UNCOMPRESSED__

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/media/Image.h"
class RGBPixel;

/**
Current class is a specific low level implementation of an uncompressed high
dynamic range RGB image: each pixel keeps its red, green and blue components
as unbounded double precision floating point values (e.g. radiance values
computed by a global illumination renderer), ordered in a sequential array
of RGB triplets, row by row from the upper left pixel, and left to right on
each row.

As an `Image`, the RGBPixel based methods map the range [0, 1] of the
components to the byte range [0, 255]: values outside [0, 1] are clamped
when reading, so this image can be given to the 8 bit exporters (PPM, PNG,
JPEG) of `ImagePersistence`. The full range of the components is kept by
`putPixel` / `getPixel` and by the high dynamic range exporters (RGBE).

On Turbo C++ the components are kept in the program memory, so the image
must fit in one 64 KB segment (i.e. 52 x 52 pixels): `init` fails for
larger ones.
*/
class RGBImageFloatUncompressed : public Image {

private:
    enum { COMPONENTS_PER_PIXEL = 3 };

    double* data;
    int xSize;
    int ySize;

    long pixelBaseIndex(int x, int y) const;
    static char componentToByte(double value);

public:
    static const Class CLASS;
    virtual const Class *getClass() const;

    RGBImageFloatUncompressed();
    virtual ~RGBImageFloatUncompressed();

    virtual int getSizeInBytes() const;

    /// Allocates the image with all of its components set to zero
    virtual BOOLEAN init(int width, int height);

    virtual BOOLEAN initNoFill(int width, int height);

    void putPixel(int x, int y, double r, double g, double b);
    void putPixel(int x, int y, const ColorRgb& color);
    ColorRgb getPixel(int x, int y) const;

    virtual void putPixelRgb(int x, int y, RGBPixel* p);

    virtual RGBPixel* getPixelRgb(int x, int y) const;

    virtual void getPixelRgb(int x, int y, RGBPixel* p) const;

    virtual int getXSize() const;

    virtual int getYSize() const;

    virtual void dispose();
};

#endif
