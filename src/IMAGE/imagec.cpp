/**
Copyright (c) 1994-2000 K.U.Leuven

This software is provided AS IS, without any express or implied
warranty.  In no event will the authors or the K.U.Leuven be held
liable for any damages or loss of profit arising from the use or
non-fitness for a particular purpose of this software.

See file 0README in the home directory of RenderPark for details about
copyrights and licensing.
===========================================================================
NAME:       image
TYPE:       c++ code
PROJECT:    Render park - Image output
CONTENT:    ANSI-C interface to the image output library
===========================================================================
AUTHORS:    pb      Philippe Bekaert
            jp      Jan Prikryl
===========================================================================
HISTORY:

06-Mar-00 11:23:34  jp      last modification
06-Mar-00 11:22:51  jp      typo in NEW_TIFF_GENERAL_HANDLE for no TIFF
20-Sep-99 21:51:43  jp      changes for lib tiff autoconf
03-Oct-98 15:26:04  pb      released
*/

#include <cstring>

#include "common/error.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "IMAGE/imagecpp.h"
#include "IMAGE/ppm.h"
#include "IMAGE/pic.h"

/**
RGB x LOG LUV TIFF MACROS
*/

#define PRE_TIFF_GENERAL_HANDLE(_func) \
    logError(_func, "TIFF support has not been compiled in")

ImageOutputHandle::ImageOutputHandle(): width(), height(), driverName(), gamma() {
}

int
ImageOutputHandle::writeDisplayRGB(unsigned char * /*x*/) {
    fprintf(stderr, "%s does not support display RGB output.\n", driverName);
    return 0;
}

inline void
gammaCorrect(RGB &rgb, const float gamma[3]) {
  rgb.r = gamma[0] == 1.0 ? rgb.r : std::pow(rgb.r, 1.0f / gamma[0]);
  rgb.g = gamma[1] == 1.0 ? rgb.g : std::pow(rgb.g, 1.0f / gamma[1]);
  rgb.b = gamma[2] == 1.0 ? rgb.b : std::pow(rgb.b, 1.0f / gamma[2]);
}

int
ImageOutputHandle::writeDisplayRGB(float *rgbFloatArray) {
    unsigned char *rgb = new unsigned char[3 * width];
    for ( int i = 0; i < width; i++ ) {
        // Convert RGB radiance to display RGB
        RGB displayRgb = *(RGB *) (&rgbFloatArray[3 * i]);
        // Apply gamma correction
        gammaCorrect(displayRgb, gamma);
        // Convert float to byte representation
        rgb[3 * i] = (unsigned char) (displayRgb.r * 255.0);
        rgb[3 * i + 1] = (unsigned char) (displayRgb.g * 255.0);
        rgb[3 * i + 2] = (unsigned char) (displayRgb.b * 255.0);
    }

    // Output display RGB values
    int pixelsWriten = writeDisplayRGB(rgb);

    delete[] rgb;
    return pixelsWriten;
}

int
ImageOutputHandle::writeRadianceRGB(float *rgbRadiance) {
    unsigned char *rgb = new unsigned char[3 * width];
    for ( int i = 0; i < width; i++ ) {
        // Convert RGB radiance to display RGB
        RGB displayRgb{};
        radianceToRgb(*(COLOR *) &rgbRadiance[3 * i], &displayRgb);
        // Apply gamma correction
        gammaCorrect(displayRgb, gamma);
        // Convert float to byte representation
        rgb[3 * i] = (unsigned char) (displayRgb.r * 255.0);
        rgb[3 * i + 1] = (unsigned char) (displayRgb.g * 255.0);
        rgb[3 * i + 2] = (unsigned char) (displayRgb.b * 255.0);
    }

    // Output display RGB values
    int pixelsWriten = writeDisplayRGB(rgb);

    delete[] rgb;
    return pixelsWriten;
}

/**
Returns file name extension. Understands extra suffixes ".Z", ".gz",
".bz", and ".bz2".
*/
char *
imageFileExtension(char *fileName) {
    char *ext = fileName + strlen(fileName) - 1; // Find filename extension

    while ( ext >= fileName && *ext != '.' ) {
        ext--;
    }

    if ( !strcmp(ext, ".Z") ||
         !strcmp(ext, ".gz") ||
         !strcmp(ext, ".bz") ||
         !strcmp(ext, ".bz2") ) {
        ext--; // Before '.'
        while ( ext >= fileName && *ext != '.' ) {
            ext--;
        }
        // Find extension before .gz or .Z
    }

    return ext + 1; // After '.'
}

/**
Examines filename extension in order to decide what file format to
use to write radiance image
*/
ImageOutputHandle *
createRadianceImageOutputHandle(
    char *fileName,
    FILE *fp,
    int isPipe,
    int width,
    int height,
    float /*referenceLuminance*/)
{
    if ( fp ) {
        char *ext = isPipe ? (char *) "ppm" : imageFileExtension(fileName);
        // Assume PPM format if pipe
        if ( strncasecmp(ext, "ppm", 3) == 0 ) {
            return new PPMOutputHandle(fp, width, height);
        }
        // Olaf: HDR PIC output
        else if ( strncasecmp(ext, "pic", 3) == 0 ) {
            if ( isPipe ) {
                logError("createRadianceImageOutputHandle",
                         "Can't write PIC output to a pipe.\n");
                return nullptr;
            }

            FILE *fd = freopen("/dev/null", "w", fp);
            if (fd == nullptr) {
                fprintf(stderr, "Warning: can not reopen /dev/null\n");
            }

            return new PicOutputHandle(fileName, width, height);
        } else {
            PRE_TIFF_GENERAL_HANDLE("createRadianceImageOutputHandle");
            logError("createRadianceImageOutputHandle",
                     "Can't save high dynamic range images to a '%s' file.", ext);
            return nullptr;
        }
    }
    return nullptr;
}

/**
Same, but for writing "normal" display RGB images instead radiance image
*/
ImageOutputHandle *
createImageOutputHandle(
    char *fileName,
    FILE *fp,
    int isPipe,
    int width,
    int height)
{
    if ( fp ) {
        char *ext = isPipe ? (char *) "ppm" : imageFileExtension(fileName);

        if ( strncasecmp(ext, "ppm", 3) == 0 ) {
            return new PPMOutputHandle(fp, width, height);
        } else {
            PRE_TIFF_GENERAL_HANDLE("createImageOutputHandle");
            logError("createImageOutputHandle",
                     "Can't save display-RGB images to a '%s' file.\n", ext);
            return nullptr;
        }
    }
    return nullptr;
}

/**
Write a scanline of display RGB, RGB radiance or CIE XYZ radiance data.
3 samples per pixel: RGB order for RGB data and XYZ order for CIE XYZ data
*/
int
writeDisplayRGB(ImageOutputHandle *img, unsigned char *data) {
    return img->writeDisplayRGB(data);
}

/**
Finish writing the image
*/
void
deleteImageOutputHandle(ImageOutputHandle *img) {
    delete img;
}
