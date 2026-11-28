#ifndef __IMAGE_OUTPUT_TARGET__
#define __IMAGE_OUTPUT_TARGET__

#include "java/io/OutputStream.h"
class Image;
class RGBImageFloatUncompressed;

/**
Destination of a rendered image: an open output stream and the file format
written on it, chosen from the extension of the file name. The images are
written by VITRAL's ImagePersistence:
- PPM and PNG: display RGB (tone mapped and gamma corrected by ScreenBuffer)
- PIC: the high dynamic range radiances, in Radiance's RGBE format
The stream belongs to the caller, which closes it after the target is
deleted.
*/
class ImageOutputTarget {
  public:
    enum Format {
        DISPLAY_PPM,
        DISPLAY_PNG,
        RADIANCE_RGBE
    };

  private:
    OutputStream *outputStream;
    Format format;
    const char *driverName;

    ImageOutputTarget(OutputStream *inOutputStream, Format inFormat, const char *inDriverName);

  public:
    /**
    Examines the file name extension in order to decide what file format to
    use (PPM when the stream is a pipe, unless the name is of a PNG file).
    Returns NULL, after reporting it,
    for an extension of an unsupported format or a null stream.
    */
    static ImageOutputTarget *create(const char *fileName, OutputStream *outputStream, int isPipe);

    /**
    Returns file name extension. Understands extra suffixes ".Z", ".gz",
    ".bz", and ".bz2".
    */
    static const char *imageFileExtension(const char *fileName);

    Format getFormat() const;
    const char *getDriverName() const;

    bool writeDisplayImage(Image *image) const;
    bool writeRadianceImage(const RGBImageFloatUncompressed *image) const;
};

#endif
