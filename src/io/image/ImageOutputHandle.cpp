/**
Philippe Bekaert & Jan Prikryl, October 1998 - March 2000
*/
#include <cstring>

#include "java/lang/Math.h"
#include "common/error.h"
#include "tonemap/ToneMap.h"
#include "io/image/ppm.h"
#include "io/image/pic.h"
#include "io/image/ImageOutputHandle.h"

ImageOutputHandle::ImageOutputHandle(): width(), height(), driverName(), gamma() {
}

int
ImageOutputHandle::writeDisplayRGB(unsigned char * /*x*/) {
    fprintf(stderr, "%s does not support display RGB output.\n", driverName);
    return 0;
}

inline void
gammaCorrect(ColorRgb &rgb, const float gamma[3]) {
  rgb.r = gamma[0] == 1.0 ? rgb.r : java::Math::pow(rgb.r, 1.0f / gamma[0]);
  rgb.g = gamma[1] == 1.0 ? rgb.g : java::Math::pow(rgb.g, 1.0f / gamma[1]);
  rgb.b = gamma[2] == 1.0 ? rgb.b : java::Math::pow(rgb.b, 1.0f / gamma[2]);
}

int
ImageOutputHandle::writeDisplayRGB(float *rgbFloatArray) {
    unsigned char *rgb = new unsigned char[3 * width];
    for ( int i = 0; i < width; i++ ) {
        // Convert RGB radiance to display RGB
        ColorRgb displayRgb = *(ColorRgb *)(&rgbFloatArray[3 * i]);
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
Writes a scanline of raw radiance data
returns the number of pixels written
*/
int
ImageOutputHandle::writeRadianceRGB(ColorRgb *rgbRadiance) {
    unsigned char *rgb = new unsigned char[3 * width];
    for ( int i = 0; i < width; i++ ) {
        // Convert RGB radiance to display RGB
        ColorRgb displayRgb{};
        radianceToRgb(rgbRadiance[i], &displayRgb);

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
const char *
imageFileExtension(const char *fileName) {
    const char *fileExtension = fileName + strlen(fileName) - 1; // Find filename extension

    while ( fileExtension >= fileName && *fileExtension != '.' ) {
        fileExtension--;
    }

    if ( !strcmp(fileExtension, ".Z") ||
         !strcmp(fileExtension, ".gz") ||
         !strcmp(fileExtension, ".bz") ||
         !strcmp(fileExtension, ".bz2") ) {
        fileExtension--; // Before '.'
        while ( fileExtension >= fileName && *fileExtension != '.' ) {
            fileExtension--;
        }
        // Find extension before .gz or .Z
    }

    return fileExtension + 1; // After '.'
}

/**
Examines filename extension in order to decide what file format to
use to write radiance image
*/
ImageOutputHandle *
createRadianceImageOutputHandle(
    const char *fileName,
    FILE *fileDescriptor,
    int isPipe,
    int width,
    int height)
{
    if ( fileDescriptor ) {
        const char *fileExtension = isPipe ? "ppm" : imageFileExtension(fileName);
        // Assume PPM format if pipe
        if ( strncasecmp(fileExtension, "ppm", 3) == 0 ) {
            return new PPMOutputHandle(fileDescriptor, width, height);
        }
        // Olaf: HDR PIC output
        else if ( strncasecmp(fileExtension, "pic", 3) == 0 ) {
            if ( isPipe ) {
                logError("createRadianceImageOutputHandle",
                         "Can't write PIC output to a pipe.\n");
                return nullptr;
            }

            return new PicOutputHandle(fileName, width, height);
        } else {
            logError("createRadianceImageOutputHandle",
                     "Can't save high dynamic range image to a '%s' file, format not supported.",
                     fileExtension);
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
    const char *fileName,
    FILE *fileDescriptor,
    const int isPipe,
    const int width,
    const int height)
{
    if ( fileDescriptor ) {
        const char *fileExtension = isPipe ? "ppm" : imageFileExtension(fileName);

        if ( strncasecmp(fileExtension, "ppm", 3) == 0 ) {
            return new PPMOutputHandle(fileDescriptor, width, height);
        } else {
            logError("createImageOutputHandle",
                     "Can't save display-RGB images to a '%s' file, format not supported.\n",
                     fileExtension);
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
