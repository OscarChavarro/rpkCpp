#ifndef __IMAGE_PERSISTENCE__
#define __IMAGE_PERSISTENCE__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/io/File.h"
#include "java/io/OutputStream.h"
#include "java/lang/String.h"
class Image;
class RGBImageFloatUncompressed;
class RGBImageUncompressed;
class RGBAImageUncompressed;
class RGBAImageCompressed;
class IndexedColorImageUncompressed;

/**
This class is a front end through which images of various formats can be
exported and/or imported to/from files.

This class follows a Singleton design pattern.
*/
class ImagePersistence {

public:
    /**
    Given the filename of an input data file which contains an image, this
    method tries to recognize the file format and load the contents of it
    to the image.

    @param inImageFd - The file containing the image
    @return An RGBImageUncompressed entity that contains the image loaded in memory.
    */
    static RGBImageUncompressed* importRGB(const File& inImageFd);

    static IndexedColorImageUncompressed* importIndexedColor(const File& inImageFd);

    /**
    Given the filename of an input data file which contains an image, this
    method tries to recognize the file format and load the contents of it
    to the image.

    @param inImageFd - The file containing the image
    @return An RGBAImageUncompressed entity that contains the image loaded in memory.
    */
    static RGBAImageUncompressed* importRGBA(const File& inImageFd);

    /**
    Generic image import that may return compressed or uncompressed images
    depending on format support.
    */
    static Image* importImage(const File& inImageFd);

    /**
    This method writes the contents of the specified image to a file in
    binary RGB PPM format (i.e. P6 PPM sub-format). Returns true if everything
    works fine, false if something fails.
    @param fd
    @param img
    @return true if export was successful, false otherwise
    */
    static BOOLEAN exportPPM(const File& fd, Image* img);

    /**
    Same as exportPPM over a file, but writing on the given stream (e.g. a
    compressed or piped stream), which is not closed.
    @param os
    @param img
    @return true if export was successful, false otherwise
    */
    static BOOLEAN exportPPM(OutputStream& os, Image* img);

    /**
    This method writes the contents of the specified high dynamic range image
    to a file in the RGBE format of the Radiance system (.pic or .hdr files),
    keeping the unbounded range of its components (see ImagePersistenceRgbe).
    @param fd
    @param img
    @return true if export was successful, false otherwise
    */
    static BOOLEAN exportRGBE(const File& fd, const RGBImageFloatUncompressed* img);

    /**
    Same as exportRGBE over a file, but writing on the given stream, which is
    not closed.
    @param os
    @param img
    @return true if export was successful, false otherwise
    */
    static BOOLEAN exportRGBE(OutputStream& os, const RGBImageFloatUncompressed* img);

    /**
    Loads a high dynamic range image from a file in the RGBE format of the
    Radiance system (.pic or .hdr files).
    @param inImageFd - The file containing the image
    @return the image, to be deleted by the caller, or NULL if the file
    could not be read as an RGBE image
    */
    static RGBImageFloatUncompressed* importRGBE(const File& inImageFd);

    /**
    This method writes the contents of the specified image to a file in
    JPEG format. Returns true if everything works fine, false if something fails.
    JPEG support must be compiled with -DWITH_JPEG=ON.
    @param fd
    @param img
    @param quality JPEG quality (0-100)
    @return true if export was successful, false otherwise
    */
    static BOOLEAN exportJPEG(const File& fd, Image* img, int quality = 85);

    /**
    This method writes the contents of the specified image to a file in
    PNG format. Returns true if everything works fine, false if something fails.
    PNG support must be compiled with -DWITH_PNG=ON.
    @param fd
    @param img
    @return true if export was successful, false otherwise
    */
    static BOOLEAN exportPNG(const File& fd, Image* img);

    /**
    Same as exportPNG over a file, but writing on the given stream (e.g. a
    compressed or piped stream), which is not closed.
    PNG support must be compiled with -DWITH_PNG=ON.
    @param os
    @param img
    @return true if export was successful, false otherwise
    */
    static BOOLEAN exportPNG(OutputStream& os, Image* img);

private:
    static BOOLEAN isTextComment(const String& line);
    static String* extractExtensionFromFile(const File& fd);
};

#endif
