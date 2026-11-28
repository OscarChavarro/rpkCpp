#ifndef __RGB_IMAGE_FLOAT_UNCOMPRESSED__
#define __RGB_IMAGE_FLOAT_UNCOMPRESSED__

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
*/
class RGBImageFloatUncompressed : public Image {

private:
    static const int COMPONENTS_PER_PIXEL = 3;

    double* data;
    int xSize;
    int ySize;

    int pixelBaseIndex(int x, int y) const;
    static char componentToByte(double value);

public:
    RGBImageFloatUncompressed();
    virtual ~RGBImageFloatUncompressed();

    virtual int getSizeInBytes() const override;

    /// Allocates the image with all of its components set to zero
    virtual bool init(int width, int height) override;

    virtual bool initNoFill(int width, int height) override;

    void putPixel(int x, int y, double r, double g, double b);
    void putPixel(int x, int y, const ColorRgb& color);
    ColorRgb getPixel(int x, int y) const;

    virtual void putPixelRgb(int x, int y, RGBPixel* p) override;

    virtual RGBPixel* getPixelRgb(int x, int y) const override;

    virtual void getPixelRgb(int x, int y, RGBPixel* p) const override;

    virtual int getXSize() const override;

    virtual int getYSize() const override;

    virtual void dispose() override;
};

#endif
