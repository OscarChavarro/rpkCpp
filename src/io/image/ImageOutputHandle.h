/**
Interface for writing image data in different file formats
*/

#ifndef __IMAGE_OUTPUT_HANDLE__
#define __IMAGE_OUTPUT_HANDLE__

#include "java/io/OutputStream.h"
#include "common/ColorRgb.h"
#include "tonemap/ToneMappingContext.h"

class ImageOutputHandle {
  protected:
    int width;
    int height;

    void init(const char *_name, int _width, int _height);
    static void gammaCorrect(ColorRgb &rgb, const float gamma[3]);

  public:
    ImageOutputHandle();

    virtual ~ImageOutputHandle() {};

    // Image file output driver name
    const char *driverName;

    // Gamma correction factors for red, green and blue  used by default
    float gamma[3];
    const ToneMappingContext *toneMapOptions;

    // Writes a scanline of gamma-corrected display RGB pixels
    // returns the number of pixels written
    virtual int writeDisplayRGB(unsigned char *rgb);

    virtual int writeDisplayRGB(float *rgbFloatArray);

    virtual int writeRadianceRGB(ColorRgb *rgbRadiance);
    void setToneMappingContext(const ToneMappingContext *inToneMapOptions);

    static ImageOutputHandle *
    createRadianceImageOutputHandle(
        const char *fileName,
        java::OutputStream *outputStream,
        int isPipe,
        int width,
        int height);

    static ImageOutputHandle *
    createImageOutputHandle(
        const char *fileName,
        java::OutputStream *outputStream,
        int isPipe,
        int width,
        int height);

    static const char *imageFileExtension(const char *fileName);
    static int writeDisplayRGB(ImageOutputHandle *img, unsigned char *data);
    static void deleteImageOutputHandle(ImageOutputHandle *img);
};

inline void
ImageOutputHandle::init(const char *_name, int _width, int _height) {
    driverName = _name;
    width = _width;
    height = _height;
    gamma[0] = 1.0;
    gamma[1] = 1.0;
    gamma[2] = 1.0;
    toneMapOptions = nullptr;
}

/**
The following ImageOutputHandle constructors are only needed if you want to specify
yourself what format to use
*/

#endif
