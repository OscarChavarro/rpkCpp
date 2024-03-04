/**
Interface for writing image data in different file formats
*/

#ifndef _IMAGE_H_
#define _IMAGE_H_

#define IS_TIFF_LOGLUV_EXT(_ext) \
    (!strncasecmp(_ext,"logluv",6))

class ImageOutputHandle {
protected:
    int width;
    int height;

    void init(const char *_name, int _width, int _height) {
        drivername = _name;
        width = _width;
        height = _height;
        gamma[0] = gamma[1] = gamma[2] = 1.;
    }

public:
    ImageOutputHandle();

    virtual ~ImageOutputHandle() {};

    /* image file output driver name */
    const char *drivername;

    /* gamma correction factors for red, green and blue  used by default  */
    /* WriteRadianceRGB() */
    float gamma[3];

    /* writes a scanline of gamma-corrected display RGB pixels */
    /* returns the number of pixels written. */
    virtual int writeDisplayRGB(unsigned char *rgb);

    virtual int writeDisplayRGB(float *rgbflt);

    /* writes a scanline of raw radiance data */
    /* returns the number of pixels written. */
    virtual int writeRadianceRGB(float *rgbrad);    /* RGB radiance data */
};

#include <cstdio>

/* Examines filename extension in order to decide what file format to use to write
 * radiance image.*/
extern ImageOutputHandle *createRadianceImageOutputHandle(char *fileName, FILE *fp, int isPipe,
                                                          int width, int height,
                                                          float referenceLuminance);

/* Same, but for writing "normal" display RGB images instead radiance image. */
extern ImageOutputHandle *createImageOutputHandle(char *fileName, FILE *fp, int isPipe,
                                                  int width, int height);

#endif
