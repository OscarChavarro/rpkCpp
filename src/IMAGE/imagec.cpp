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
PROJECT:    Renderpark - Image output
CONTENT:    ANSI-C interface to the image output library
===========================================================================
AUTHORS:    pb      Philippe Bekaert
            jp      Jan Prikryl
===========================================================================
HISTORY:

06-Mar-00 11:23:34  jp      last modification
06-Mar-00 11:22:51  jp      typo in NEW_TIFF_GENERAL_HANDLE for no TIFF
20-Sep-99 21:51:43  jp      changes for libtiff autoconf
03-Oct-98 15:26:04  pb      released
*/

#include <cstring>

#include "common/error.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "IMAGE/imagecpp.h"
#include "IMAGE/ppm.h"
#include "IMAGE/pic.h"

/**
RGB x LOGLUV TIFF MACROS
*/

#define PRE_TIFF_GENERAL_HANDLE(_func) \
    logError(_func, "TIFF support has not been compiled in")

/**
DEFAULT IMPLEMENTATIONS
*/
int
ImageOutputHandle::writeDisplayRGB(unsigned char * /*x*/) {
    static bool wgiv = false;
    if ( !wgiv ) {
        fprintf(stderr, "%s does not support display RGB output.\n", drivername);
        wgiv = true;
    }
    return 0;
}

#define GAMMACORRECT(rgb, gamma) { \
  (rgb).r = (gamma)[0] == 1. ? (rgb).r : pow((rgb).r, 1./(gamma)[0]); \
  (rgb).g = (gamma)[1] == 1. ? (rgb).g : pow((rgb).g, 1./(gamma)[1]); \
  (rgb).b = (gamma)[2] == 1. ? (rgb).b : pow((rgb).b, 1./(gamma)[2]); \
}

int
ImageOutputHandle::writeDisplayRGB(float *rgbflt) {
    unsigned char *rgb = new unsigned char[3 * width];
    for ( int i = 0; i < width; i++ ) {
        // convert RGB radiance to display RGB
        RGB disprgb = *(RGB *) (&rgbflt[3 * i]);
        // radianceToRgb(*(COLOR *)&rgbflt[3*i], &disprgb);
        // apply gamma correction
        GAMMACORRECT(disprgb, gamma);
        // convert float to byte representation
        rgb[3 * i] = (unsigned char) (disprgb.r * 255.);
        rgb[3 * i + 1] = (unsigned char) (disprgb.g * 255.);
        rgb[3 * i + 2] = (unsigned char) (disprgb.b * 255.);
    }

    // Output display RGB values
    int pixwrit = writeDisplayRGB(rgb);

    delete[] rgb;
    return pixwrit;
}

int
ImageOutputHandle::writeRadianceRGB(float *rgbrad) {
    unsigned char *rgb = new unsigned char[3 * width];
    for ( int i = 0; i < width; i++ ) {
        // Convert RGB radiance to display RGB
        RGB disprgb;
        radianceToRgb(*(COLOR *) &rgbrad[3 * i], &disprgb);
        // Apply gamma correction
        GAMMACORRECT(disprgb, gamma);
        // Convert float to byte representation
        rgb[3 * i] = (unsigned char) (disprgb.r * 255.);
        rgb[3 * i + 1] = (unsigned char) (disprgb.g * 255.);
        rgb[3 * i + 2] = (unsigned char) (disprgb.b * 255.);
    }

    // Output display RGB values
    int pixwrit = writeDisplayRGB(rgb);

    delete[] rgb;
    return pixwrit;
}

/**
Returns file name extension. Understands extra suffixes ".Z", ".gz",
".bz", and ".bz2".
*/
char *
imageFileExtension(char *fileName) {
    char *ext = fileName + strlen(fileName) - 1;    /* find filename extension */

    while ( ext >= fileName && *ext != '.' ) {
        ext--;
    }

    if ((!strcmp(ext, ".Z")) ||
        (!strcmp(ext, ".gz")) ||
        (!strcmp(ext, ".bz")) ||
        (!strcmp(ext, ".bz2"))) {
        ext--; // before '.'
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
                return (ImageOutputHandle *) 0;
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
            return (ImageOutputHandle *) 0;
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
            return (ImageOutputHandle *) 0;
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
