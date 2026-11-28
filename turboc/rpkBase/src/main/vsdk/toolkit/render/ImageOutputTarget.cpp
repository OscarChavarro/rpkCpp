/**
Philippe Bekaert & Jan Prikryl, October 1998 - March 2000
*/
#include <string.h>

#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/logging/LoggerFormat.h"
#include "vsdk/toolkit/io/image/ImagePersistence.h"
#include "vsdk/toolkit/render/ImageOutputTarget.h"

ImageOutputTarget::ImageOutputTarget(OutputStream *inOutputStream, Format inFormat, const char *inDriverName):
    outputStream(inOutputStream), format(inFormat), driverName(inDriverName)
{
}

ImageOutputTarget *
ImageOutputTarget::create(const char *fileName, OutputStream *outputStream, int isPipe) {
    if ( outputStream == NULL ) {
        return NULL;
    }

    // Assume PPM format if pipe, unless it is a PNG file (e.g. compressed)
    const char *fileExtension = imageFileExtension(fileName);
    if ( strncasecmp(fileExtension, "png", 3) == 0 ) {
        return new ImageOutputTarget(outputStream, DISPLAY_PNG, "PNG");
    }
    if ( isPipe ) {
        fileExtension = "ppm";
    }
    if ( strncasecmp(fileExtension, "ppm", 3) == 0 ) {
        return new ImageOutputTarget(outputStream, DISPLAY_PPM, "PPM");
    }
    if ( strncasecmp(fileExtension, "pic", 3) == 0 ) {
        // Olaf: HDR PIC output
        return new ImageOutputTarget(outputStream, RADIANCE_RGBE, "high dynamic range PIC");
    }
    Logger::reportMessage(
        "ImageOutputTarget",
        Logger::ERROR,
        "create",
        LoggerFormat::format("Can't save high dynamic range image to a '%s' file, format not supported.", fileExtension));
    return NULL;
}

const char *
ImageOutputTarget::imageFileExtension(const char *fileName) {
    const int fileNameLength = ((int)(strlen(fileName)));
    if ( fileNameLength <= 0 ) {
        return fileName;
    }

    int extensionDotIndex = fileNameLength - 1;
    while ( extensionDotIndex >= 0 && fileName[extensionDotIndex] != '.' ) {
        extensionDotIndex--;
    }

    if ( extensionDotIndex < 0 ) {
        return fileName;
    }

    const char *fileExtension = &fileName[extensionDotIndex];
    if ( !strcmp(fileExtension, ".Z") || !strcmp(fileExtension, ".gz") || !strcmp(fileExtension, ".bz") || !strcmp(fileExtension, ".bz2") ) {
        extensionDotIndex--;
        while ( extensionDotIndex >= 0 && fileName[extensionDotIndex] != '.' ) {
            extensionDotIndex--;
        }
        if ( extensionDotIndex < 0 ) {
            return fileName;
        }
    }

    return &fileName[extensionDotIndex + 1];
}

ImageOutputTarget::Format
ImageOutputTarget::getFormat() const {
    return format;
}

const char *
ImageOutputTarget::getDriverName() const {
    return driverName;
}

bool
ImageOutputTarget::writeDisplayImage(Image *image) const {
    if ( format == DISPLAY_PNG ) {
        return ImagePersistence::exportPNG(*outputStream, image) ? true : false;
    }
    return ImagePersistence::exportPPM(*outputStream, image) ? true : false;
}

bool
ImageOutputTarget::writeRadianceImage(const RGBImageFloatUncompressed *image) const {
    return ImagePersistence::exportRGBE(*outputStream, image) ? true : false;
}
