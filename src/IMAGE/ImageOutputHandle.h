/**
Interface for writing image data in different file formats
*/

#ifndef __IMAGE_OUTPUT_HANDLE__
#define __IMAGE_OUTPUT_HANDLE__

#include <cstdio>

class ImageOutputHandle {
  protected:
    int width;
    int height;

    void init(const char *_name, int _width, int _height) {
        driverName = _name;
        width = _width;
        height = _height;
        gamma[0] = 1.0;
        gamma[1] = 1.0;
        gamma[2] = 1.0;
    }

  public:
    ImageOutputHandle();

    virtual ~ImageOutputHandle() {};

    // Image file output driver name
    const char *driverName;

    // Gamma correction factors for red, green and blue  used by default
    float gamma[3];

    // Writes a scanline of gamma-corrected display RGB pixels
    // returns the number of pixels written
    virtual int writeDisplayRGB(unsigned char *rgb);

    virtual int writeDisplayRGB(float *rgbFloatArray);

    // Writes a scanline of raw radiance data
    // returns the number of pixels written
    virtual int writeRadianceRGB(float *rgbRadiance); // RGB radiance data
};

extern ImageOutputHandle *
createRadianceImageOutputHandle(
    const char *fileName,
    FILE *fp,
    int isPipe,
    int width,
    int height,
    float referenceLuminance);

extern ImageOutputHandle *
createImageOutputHandle(
    const char *fileName,
    FILE *fp,
    int isPipe,
    int width,
    int height);

/**
The following ImageOutputHandle constructors are only needed if you want to specify
yourself what format to use
*/

extern const char *imageFileExtension(const char *fileName);
extern int writeDisplayRGB(ImageOutputHandle *img, unsigned char *data);
extern void deleteImageOutputHandle(ImageOutputHandle *img);

#endif
