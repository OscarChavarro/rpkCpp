#ifndef __IMAGE__
#define __IMAGE__

#include "vsdk/toolkit/media/MediaEntity.h"
class RGBPixel;
class ColorRgb;

/**
This abstract class establishes the required interface for all Image classes
in the VSDK toolkit, and provides some common utilities for nearest and
bi-linear interpolation evaluation on Rgb space.
*/
class Image : public MediaEntity {

public:
    virtual ~Image() = default;

    /**
    Given the width and height of the desired new size for this image, this
    method is responsible of allocating the necessary memory to keep such
    an image.
    @param width - desired new width in pixels for the image. Must be greater
    than 0.
    @param height - desired new height in pixels for the image. Must be greater
    than 0.
    @return true if image memory could be allocated, false otherwise
    */
    virtual bool init(int width, int height) = 0;

    /**
    Given the width and height of the desired new size for this image, this
    method is responsible of allocating the necessary memory to keep such
    an image.
    @param width - desired new width in pixels for the image. Must be greater
    than 0.
    @param height - desired new height in pixels for the image. Must be greater
    than 0.
    @return true if image memory could be allocated, false otherwise
    */
    virtual bool initNoFill(int width, int height) = 0;

    /**
    Returns current image width in pixels
    @return current image width in pixels
    */
    virtual int getXSize() const = 0;

    /**
    Returns current image height in pixels
    @return current image height in pixels
    */
    virtual int getYSize() const = 0;

    /**
    Given an image position inside its current boundaries and an RGBPixel,
    this method convert the pixel RGB value to its internal colorspace,
    and updates corresponding internal pixel value.
    @param x - x coordinate of desired pixel, must be between 0 and image
    width minus 1
    @param y - y coordinate of desired pixel, must be between 0 and image
    height minus 1
    @param p
    */
    virtual void putPixelRgb(int x, int y, RGBPixel* p) = 0;

    /**
    Given an image position inside its current boundaries, an RGBPixel is
    returned from internal color space representation.
    @param x - x coordinate of desired pixel, must be between 0 and image
    width minus 1
    @param y - y coordinate of desired pixel, must be between 0 and image
    height minus 1
    @return the RGBPixel corresponding to requested pixel coordinate
    inside the image.
    */
    virtual RGBPixel* getPixelRgb(int x, int y) const = 0;

    /**
    Given an image position inside its current boundaries, an RGBPixel is
    returned from internal color space representation.
    @param x - x coordinate of desired pixel, must be between 0 and image
    width minus 1
    @param y - y coordinate of desired pixel, must be between 0 and image
    height minus 1
    Modifies the RGBPixel corresponding to requested pixel coordinate
    inside the image.
    @param p
    */
    virtual void getPixelRgb(int x, int y, RGBPixel* p) const = 0;

    /**
    Given an image position inside its current boundaries, an RGBPixel is
    returned from internal color space representation.
    @param x - x coordinate of desired pixel, must be between 0 and image
    width minus 1
    @param y - y coordinate of desired pixel, must be between 0 and image
    height minus 1
    @return the RGBPixel corresponding to requested pixel coordinate
    inside the image.
    */
    char getPixel8bitGrayScale(int x, int y) const;

    /**
    Given a double value inside the integer limits of this image, this
    method returns a rgb color corresponding to the nearest getPixelRgb.

    @param x
    @param y
    @return
    */
    ColorRgb* getColorRgbNearest(double x, double y) const;

    /**
    Given a double value inside the integer limits of this image, this
    method returns a rgb color corresponding to a bi-linear interpolation
    of the 4 neighboring pixels of the float position.

    Current implementation is based on bilinear interpolation algorithm
    proposed for the bumpmap equivalent in [BLIN1978b].
    @param x
    @param y
    @return
    */
    ColorRgb* getColorRgbBiLinear(double x, double y) const;

    /**
    This method creates a checker board like visual test pattern with centered
    and colored crossing lines. This is provided as a quick image builder in
    RGB space to test image algorithm.
    */
    void createTestPattern();

private:
    static int positiveMod(int value, int modulus);
};

#endif
