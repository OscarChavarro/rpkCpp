#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "java/io/BufferedInputStream.h"
#include "java/io/BufferedOutputStream.h"
#include "java/io/File.h"
#include "java/io/FileInputStream.h"
#include "java/io/FileOutputStream.h"
#include "java/lang/String.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/io/PersistenceElement.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/media/GrayScalePalette.h"
#include "vsdk/toolkit/media/IndexedColorImageUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageCompressed.h"
#include "vsdk/toolkit/media/RGBAImageUncompressed.h"
#include "vsdk/toolkit/media/RGBImageUncompressed.h"
#include "vsdk/toolkit/media/RGBPixel.h"
#include "vsdk/toolkit/io/image/ImagePersistence.h"
#include "vsdk/toolkit/io/image/ImagePersistenceRgbe.h"
#include "vsdk/toolkit/media/RGBImageFloatUncompressed.h"
int readIntLE(const unsigned char* data, int offset) {
    return (data[offset] & 0xFF) |
           ((data[offset + 1] & 0xFF) << 8) |
           ((data[offset + 2] & 0xFF) << 16) |
           ((data[offset + 3] & 0xFF) << 24);
}

int ddsFourCCToCompressionFormat(const char fourCC[5]) {
    if (strncmp(fourCC, "DXT1", 4) == 0) {
        return RGBAImageCompressed::COMPRESSION_DXT1;
    }
    if (strncmp(fourCC, "DXT3", 4) == 0) {
        return RGBAImageCompressed::COMPRESSION_DXT3;
    }
    if (strncmp(fourCC, "DXT5", 4) == 0) {
        return RGBAImageCompressed::COMPRESSION_DXT5;
    }
    return RGBAImageCompressed::COMPRESSION_UNKNOWN;
}

unsigned short readU16BE(const unsigned char* p) {
    return (unsigned short)((p[0] << 8) | p[1]);
}

unsigned int readU32BE(const unsigned char* p) {
    return ((unsigned int)p[0] << 24) |
           ((unsigned int)p[1] << 16) |
           ((unsigned int)p[2] << 8) |
           ((unsigned int)p[3]);
}

IndexedColorImageUncompressed* importSgiIndexed(const File& inImageFd)
{
    String filePath = inImageFd.getPath();
    const String platformName = File::toPlatformName(filePath);
    FILE* f = fopen(platformName.toCString(), "rb");
    if (f == NULL) return NULL;

    unsigned char header[512];
    if (fread(header, 1, sizeof(header), f) != sizeof(header)) {
        fclose(f);
        return NULL;
    }

    const unsigned short magic = readU16BE(&header[0]);
    const unsigned char storageFormat = header[2];
    const unsigned char bytesPerChannel = header[3];
    const unsigned short dimensions = readU16BE(&header[4]);
    const unsigned short xSize = readU16BE(&header[6]);
    const unsigned short ySize = readU16BE(&header[8]);
    const unsigned short channels = readU16BE(&header[10]);
    const unsigned int colormapId = readU32BE(&header[104]);

    if (magic != 474 || storageFormat != 0x01 || bytesPerChannel != 1 ||
        dimensions < 2 || channels != 1 || colormapId != 0 || xSize == 0 || ySize == 0) {
        fclose(f);
        return NULL;
    }

    const int tables = (int)ySize * (int)channels;
    ArrayList<unsigned int> starts;
    starts.reserve((long int)tables);
    long i;
    for (i = 0; i < (long int)tables; i++) starts.add(0u);
    ArrayList<unsigned int> lengths;
    lengths.reserve((long int)tables);
    for (i = 0; i < (long int)tables; i++) lengths.add(0u);
    ArrayList<unsigned char> tmp;
    tmp.reserve((long int)tables * 4);
    for (i = 0; i < (long int)tables * 4; i++) tmp.add(0u);
    if (fread(tmp.data(), 1, (size_t)tmp.size(), f) != (size_t)tmp.size()) {
        fclose(f);
        return NULL;
    }
    for (i = 0; i < tables; i++) starts[(long int)i] = readU32BE(&tmp[(long int)i * 4]);
    if (fread(tmp.data(), 1, (size_t)tmp.size(), f) != (size_t)tmp.size()) {
        fclose(f);
        return NULL;
    }
    for (i = 0; i < tables; i++) lengths[(long int)i] = readU32BE(&tmp[(long int)i * 4]);

    IndexedColorImageUncompressed* img = new IndexedColorImageUncompressed(new GrayScalePalette());
    if (!img->init((int)xSize, (int)ySize)) {
        delete img;
        fclose(f);
        return NULL;
    }

    for (int y = 0; y < (int)ySize; y++) {
        const unsigned int start = starts[(long int)y];
        const unsigned int length = lengths[(long int)y];
        if (length == 0) continue;
        if (fseek(f, (long)start, SEEK_SET) != 0) continue;
        ArrayList<unsigned char> line;
        line.reserve((long int)length);
        for (long int li = 0; li < (long int)length; li++) line.add(0u);
        if (fread(line.data(), 1, length, f) != length) continue;

        int x = 0;
        for (unsigned int pos = 0; pos < length && x < (int)xSize; pos++) {
            BOOLEAN literal = (line[(long int)pos] & 0x80) != 0;
            int count = (line[(long int)pos] & 0x7F);
            if (count == 0) break;

            if (literal) {
                for (i = 0; i < count && x < (int)xSize; i++) {
                    pos++;
                    if (pos >= length) break;
                    img->putPixel(x, (int)ySize - y - 1, (char)line[(long int)pos]);
                    x++;
                }
            }
            else {
                pos++;
                if (pos >= length) break;
                char v = (char)line[(long int)pos];
                for (i = 0; i < count && x < (int)xSize; i++) {
                    img->putPixel(x, (int)ySize - y - 1, v);
                    x++;
                }
            }
        }
    }

    fclose(f);
    return img;
}

RGBAImageCompressed* importDDSCompressed(const File& inImageFd) {
    String filePath = inImageFd.getPath();
    const String platformName = File::toPlatformName(filePath);
    FILE* f = fopen(platformName.toCString(), "rb");
    if (f == NULL) {
        fprintf(stderr, "Error: could not open DDS file \"%s\"\\n", filePath.toCString());
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fileSize < 128) {
        fclose(f);
        fprintf(stderr, "Error: DDS file too short\\n");
        return NULL;
    }

    ArrayList<unsigned char> fileData;
    fileData.reserve((long int)fileSize);
    for (long int i = 0; i < (long int)fileSize; i++) fileData.add(0u);
    size_t readCount = fread(fileData.data(), 1, (size_t)fileData.size(), f);
    fclose(f);
    if (readCount != (size_t)fileData.size()) {
        fprintf(stderr, "Error: could not read complete DDS file\\n");
        return NULL;
    }

    if (fileData[0] != 'D' || fileData[1] != 'D' ||
        fileData[2] != 'S' || fileData[3] != (unsigned char)' ') {
        fprintf(stderr, "Error: DDS signature not recognized\\n");
        return NULL;
    }

    int headerSize = readIntLE(fileData.data(), 4);
    if (headerSize != 124) {
        fprintf(stderr, "Error: DDS header size not recognized\\n");
        return NULL;
    }

    int height = readIntLE(fileData.data(), 12);
    int width = readIntLE(fileData.data(), 16);
    int pixelFormatFlags = readIntLE(fileData.data(), 80);
    char fourCC[5] = {0, 0, 0, 0, 0};
    memcpy(fourCC, fileData.data() + 84, 4);

    if ((pixelFormatFlags & 0x04) == 0) {
        fprintf(stderr, "Error: DDS file does not use a FourCC compressed format\\n");
        return NULL;
    }

    int compressionFormat = ddsFourCCToCompressionFormat(fourCC);
    if (compressionFormat == RGBAImageCompressed::COMPRESSION_UNKNOWN) {
        fprintf(stderr, "Error: DDS compressed format not supported: %.4s\\n", fourCC);
        return NULL;
    }

    int dataOffset = 128;
    int dataSize = ((int)(fileData.size())) - dataOffset;
    if (dataSize <= 0) {
        fprintf(stderr, "Error: DDS has no payload data\\n");
        return NULL;
    }

    RGBAImageCompressed* image = new RGBAImageCompressed();
    if (!image->initCompressed(width, height, compressionFormat,
                               ((char*)(fileData.data() + dataOffset)),
                               dataSize)) {
        delete image;
        fprintf(stderr, "Error: could not initialize compressed DDS image\\n");
        return NULL;
    }

    return image;
}

// The headers of libjpeg up to 6b have no C++ guards
#ifdef VITRAL_WITH_JPEG
extern "C" {
#include <jpeglib.h>
}
#endif

#ifdef VITRAL_WITH_PNG
#include <png.h>
// libpng before 1.0.6 (i.e. 1.0.3 of Tru64 UNIX 5.1) has no png_jmpbuf
#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif
// Before 1.2.9 the expansions of palettes, gray levels of 1, 2 or 4 bits and
// tRNS chunks are all done by png_set_expand (its newer, specific names are
// missing in 1.0.x)
#if defined(PNG_LIBPNG_VER) && PNG_LIBPNG_VER < 10209
#define png_set_palette_to_rgb(png_ptr) png_set_expand(png_ptr)
#define png_set_expand_gray_1_2_4_to_8(png_ptr) png_set_expand(png_ptr)
#define png_set_tRNS_to_alpha(png_ptr) png_set_expand(png_ptr)
#endif
#endif

BOOLEAN ImagePersistence::isTextComment(const String& line) {
    if (line.isEmpty()) {
        return FALSE;
    }

    int i = 0;
    while (i < line.length() &&
           (line.charAt(i) != ' ' && line.charAt(i) != '\t')) {
        i++;
    }

    return i < line.length() && line.charAt(i) == '#';
}

String* ImagePersistence::extractExtensionFromFile(const File& fd) {
    String path = fd.getName();
    const char* pathStr = path.toCString();

    const char* lastDot = strrchr(pathStr, '.');
    if (lastDot != NULL && lastDot != pathStr) {
        String* ext = new String(lastDot + 1);
        char extBuffer[256];
        strcpy(extBuffer, ext->toCString());
        for (int i = 0; i < (int)strlen(extBuffer); i++) {
            extBuffer[i] = tolower(extBuffer[i]);
        }
        delete ext;
        String* result = new String(extBuffer);
        return result;
    }
    return new String("");
}

RGBImageUncompressed* ImagePersistence::importRGB(const File& inImageFd) {
    String* type = extractExtensionFromFile(inImageFd);
    RGBImageUncompressed* retImage = NULL;

#ifdef VITRAL_WITH_JPEG
    if (type->equals("jpg") || type->equals("jpeg")) {
        String nameStr = inImageFd.getPath();
        const char* filename = nameStr.toCString();
        const String platformName = File::toPlatformName(filename);
        FILE* infile = fopen(platformName.toCString(), "rb");
        if (infile == NULL) {
            fprintf(stderr, "Cannot open JPEG file: %s\n", filename);
            delete type;
            return new RGBImageUncompressed();
        }

        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, infile);
        jpeg_read_header(&cinfo, TRUE);

        cinfo.out_color_space = JCS_RGB;
        jpeg_start_decompress(&cinfo);

        int xSize = (int)cinfo.output_width;
        int ySize = (int)cinfo.output_height;
        int channels = (int)cinfo.output_components;

        retImage = new RGBImageUncompressed();
        if (!retImage->initNoFill(xSize, ySize)) {
            fprintf(stderr, "Failed to allocate image memory for JPEG\n");
            jpeg_destroy_decompress(&cinfo);
            fclose(infile);
            delete retImage;
            delete type;
            return new RGBImageUncompressed();
        }

        unsigned char* rowBuffer = new unsigned char[xSize * channels];
        for (int y = 0; y < ySize; y++) {
            JSAMPROW rowPtr = rowBuffer;
            jpeg_read_scanlines(&cinfo, &rowPtr, 1);
            for (int x = 0; x < xSize; x++) {
                unsigned char r = rowBuffer[x * channels + 0];
                unsigned char g = (channels >= 3) ? rowBuffer[x * channels + 1] : r;
                unsigned char b = (channels >= 3) ? rowBuffer[x * channels + 2] : r;
                retImage->putPixel(x, y, (char)r, (char)g, (char)b);
            }
        }
        delete[] rowBuffer;

        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);

        delete type;
        return retImage;
    }
#endif

#ifdef VITRAL_WITH_PNG
    if (type->equals("png")) {
        String pngNameStr = inImageFd.getPath();
        const char* filename = pngNameStr.toCString();
        const String platformName = File::toPlatformName(filename);
        FILE* fp = fopen(platformName.toCString(), "rb");
        if (fp == NULL) {
            delete type;
            return new RGBImageUncompressed();
        }

        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png_ptr) {
            fclose(fp);
            delete type;
            return new RGBImageUncompressed();
        }
        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
            png_destroy_read_struct(&png_ptr, NULL, NULL);
            fclose(fp);
            delete type;
            return new RGBImageUncompressed();
        }

        if (setjmp(png_jmpbuf(png_ptr))) {
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            fclose(fp);
            delete type;
            return new RGBImageUncompressed();
        }

        png_init_io(png_ptr, fp);
        png_read_info(png_ptr, info_ptr);

        int width = (int)png_get_image_width(png_ptr, info_ptr);
        int height = (int)png_get_image_height(png_ptr, info_ptr);
        int color_type = png_get_color_type(png_ptr, info_ptr);
        int bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        if (bit_depth == 16) png_set_strip_16(png_ptr);
        if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
        if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png_ptr);
        if (color_type == PNG_COLOR_TYPE_RGB_ALPHA || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_strip_alpha(png_ptr);

        png_read_update_info(png_ptr, info_ptr);

        ArrayList<unsigned char> pixels;
        pixels.reserve((long int)width * (long int)height * 3);
        long int i;
        for (i = 0; i < (long int)width * (long int)height * 3; i++) pixels.add(0u);
        ArrayList<png_bytep> row_ptrs;
        row_ptrs.reserve((long int)height);
        for (i = 0; i < (long int)height; i++) row_ptrs.add(NULL);
        int y;
        for (y = 0; y < height; y++) {
            row_ptrs[(long int)y] = ((png_bytep)(&pixels[(long int)y * (long int)width * 3]));
        }
        png_read_image(png_ptr, row_ptrs.data());

        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);

        retImage = new RGBImageUncompressed();
        if (!retImage->initNoFill(width, height)) {
            delete retImage;
            delete type;
            return new RGBImageUncompressed();
        }

        long int pos = 0;
        for (y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                retImage->putPixel(x, y,
                    (char)pixels[pos + 0],
                    (char)pixels[pos + 1],
                    (char)pixels[pos + 2]);
                pos += 3;
            }
        }

        delete type;
        return retImage;
    }
#endif

    if (type->equals("ppm")) {
        String ppmNameStr = inImageFd.getPath();
        FileInputStream fis(ppmNameStr.toCString());
        BufferedInputStream bis(&fis);

        BOOLEAN exit = FALSE;
        char* lineStr = NULL;
        int stage = 1;
        int xSize = 0, ySize = 0;
        int i = 0;

        do {
            if (lineStr != NULL) {
                delete[] lineStr;
            }
            lineStr = PersistenceElement::readAsciiLine(bis);

            if (lineStr == NULL) {
                break;
            }

            if (strcmp(lineStr, "255") == 0) {
                exit = TRUE;
            }

            if (isTextComment(String(lineStr))) {
                continue;
            }

            switch (stage) {
                case 1:
                    if (strncmp(lineStr, "P6", 2) != 0) {
                        delete type;
                        delete[] lineStr;
                        fprintf(stderr, "Error reading internal PPM file subformat\n");
                        return new RGBImageUncompressed();
                    }
                    stage++;
                    break;

                case 2:
                    if (lineStr[0] == '#') {
                    } else {
                        char lineCopy[256];
                        strcpy(lineCopy, lineStr);

                        char* token = strtok(lineCopy, " \t");
                        if (token != NULL) {
                            xSize = atoi(token);
                            token = strtok(NULL, " \t");
                            if (token != NULL) {
                                ySize = atoi(token);
                            }
                        }
                        stage++;
                    }
                    break;
            }
        } while (!exit);

        if (lineStr != NULL) {
            delete[] lineStr;
        }

        if (xSize <= 0 || ySize <= 0) {
            fprintf(stderr, "Invalid image dimensions\n");
            delete type;
            return new RGBImageUncompressed();
        }

        retImage = new RGBImageUncompressed();
        if (!retImage->initNoFill(xSize, ySize)) {
            fprintf(stderr, "Failed to allocate image memory\n");
            delete retImage;
            delete type;
            return new RGBImageUncompressed();
        }

        char* barr = new char[xSize * 3];
        for (i = 0; i < ySize; i++) {
            unsigned char* ubarr = (unsigned char*)barr;
            PersistenceElement::readBytes(bis, ubarr, xSize * 3);
            for (int x = 0; x < xSize; x++) {
                retImage->putPixel(x, i, barr[x*3], barr[x*3+1], barr[x*3+2]);
            }
        }

        delete[] barr;

        // Invert image (flip vertically)
        for (int y = 0; y < ySize / 2; y++) {
            for (int x = 0; x < xSize; x++) {
                RGBPixel* ppa = retImage->getPixelRgb(x, y);
                RGBPixel* ppb = retImage->getPixelRgb(x, ySize - y - 1);
                if (ppa != NULL && ppb != NULL) {
                    retImage->putPixelRgb(x, y, ppb);
                    retImage->putPixelRgb(x, ySize - y - 1, ppa);
                }
                delete ppa;
                delete ppb;
            }
        }

        delete type;
        return retImage;
    }

    delete type;
    return new RGBImageUncompressed();
}

IndexedColorImageUncompressed* ImagePersistence::importIndexedColor(const File& inImageFd)
{
    String* type = extractExtensionFromFile(inImageFd);
    if (type->equals("bw") || type->equals("sgi")) {
        IndexedColorImageUncompressed* sgi = importSgiIndexed(inImageFd);
        delete type;
        if (sgi != NULL) {
            return sgi;
        }
        return new IndexedColorImageUncompressed(new GrayScalePalette());
    }
    delete type;

    RGBImageUncompressed* rgb = importRGB(inImageFd);
    IndexedColorImageUncompressed* indexed = new IndexedColorImageUncompressed();
    if (rgb == NULL || rgb->getXSize() <= 0 || rgb->getYSize() <= 0) {
        if (rgb != NULL) delete rgb;
        return indexed;
    }

    indexed->init(rgb->getXSize(), rgb->getYSize());
    for (int y = 0; y < rgb->getYSize(); y++) {
        for (int x = 0; x < rgb->getXSize(); x++) {
            RGBPixel* p = rgb->getPixelRgb(x, y);
            if (p != NULL) {
                int r = (unsigned char)p->r;
                int g = (unsigned char)p->g;
                int b = (unsigned char)p->b;
                int luma = (299 * r + 587 * g + 114 * b) / 1000;
                indexed->putPixel(x, y, (char)luma);
                delete p;
            }
        }
    }

    delete rgb;
    return indexed;
}

RGBAImageUncompressed* ImagePersistence::importRGBA(const File& inImageFd) {
#ifdef VITRAL_WITH_PNG
    String* type = extractExtensionFromFile(inImageFd);
    BOOLEAN isPng = type->equals("png");
    delete type;
    if (isPng) {
        String pngNameStr = inImageFd.getPath();
        const String platformName = File::toPlatformName(pngNameStr);
        FILE* fp = fopen(platformName.toCString(), "rb");
        if (!fp) {
            fprintf(stderr, "Cannot open PNG file: %s\n", pngNameStr.toCString());
            return NULL;
        }
        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop info_ptr = png_ptr ? png_create_info_struct(png_ptr) : NULL;
        if (!png_ptr || !info_ptr) {
            png_destroy_read_struct(&png_ptr, NULL, NULL);
            fclose(fp);
            return NULL;
        }
        if (setjmp(png_jmpbuf(png_ptr))) {
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            fclose(fp);
            return NULL;
        }
        png_init_io(png_ptr, fp);
        png_read_info(png_ptr, info_ptr);

        int width = (int)png_get_image_width(png_ptr, info_ptr);
        int height = (int)png_get_image_height(png_ptr, info_ptr);
        int color_type = png_get_color_type(png_ptr, info_ptr);
        int bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        // Everything to 8 bits RGBA: files without alpha are opaque
        if (bit_depth == 16) png_set_strip_16(png_ptr);
        if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
        if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png_ptr);
        if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_PALETTE) {
            png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
        }
        png_read_update_info(png_ptr, info_ptr);

        unsigned char* pixels =
            new unsigned char[(size_t)width * (size_t)height * 4];
        png_bytep* rows = new png_bytep[(size_t)height];
        int y;
        for (y = 0; y < height; y++) {
            rows[y] = &pixels[(size_t)y * (size_t)width * 4];
        }
        png_read_image(png_ptr, rows);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        delete[] rows;

        RGBAImageUncompressed* retImage = new RGBAImageUncompressed();
        if (!retImage->init(width, height)) {
            delete retImage;
            delete[] pixels;
            return NULL;
        }
        size_t pos = 0;
        for (y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                retImage->putPixel(x, y, (char)pixels[pos], (char)pixels[pos + 1],
                                   (char)pixels[pos + 2], (char)pixels[pos + 3]);
                pos += 4;
            }
        }
        delete[] pixels;
        return retImage;
    }
#endif

    // Formats without transparency: an opaque image
    RGBImageUncompressed* rgb = importRGB(inImageFd);
    if (rgb == NULL) {
        return NULL;
    }
    RGBAImageUncompressed* retImage = new RGBAImageUncompressed();
    retImage->init(rgb->getXSize(), rgb->getYSize());
    RGBPixel pixel;
    for (int y = 0; y < rgb->getYSize(); y++) {
        for (int x = 0; x < rgb->getXSize(); x++) {
            rgb->getPixelRgb(x, y, &pixel);
            retImage->putPixel(x, y, pixel.r, pixel.g, pixel.b, (char)255);
        }
    }
    delete rgb;
    return retImage;
}

Image* ImagePersistence::importImage(const File& inImageFd) {
    String* type = extractExtensionFromFile(inImageFd);
    if (type->equals("dds")) {
        RGBAImageCompressed* ddsImage = importDDSCompressed(inImageFd);
        delete type;
        return ddsImage;
    }

    delete type;
    return importRGB(inImageFd);
}

BOOLEAN ImagePersistence::exportPPM(const File& fd, Image* img) {
    if (img == NULL) {
        return FALSE;
    }

    String ppmExportName = fd.getPath();
    FileOutputStream fos(ppmExportName.toCString());
    BufferedOutputStream writer(&fos);
    return exportPPM(writer, img);
}

BOOLEAN ImagePersistence::exportPPM(OutputStream& os, Image* img) {
    if (img == NULL) {
        return FALSE;
    }

    String line1("P6\n");
    String line2("# Image generated by VitralSDK (https://github.com/OscarChavarro/vitral)\n");

    char sizeStr[256];
    snprintf(sizeStr, sizeof(sizeStr), "%d %d\n", img->getXSize(), img->getYSize());
    String line3(sizeStr);

    String line4("255\n");

    // Use raw ASCII writes (without trailing NUL bytes) for valid PPM header text lines.
    PersistenceElement::writeBytes(
        os,
        ((const unsigned char*)(line1.toCString())),
        ((int)(strlen(line1.toCString()))));
    PersistenceElement::writeBytes(
        os,
        ((const unsigned char*)(line2.toCString())),
        ((int)(strlen(line2.toCString()))));
    PersistenceElement::writeBytes(
        os,
        ((const unsigned char*)(line3.toCString())),
        ((int)(strlen(line3.toCString()))));
    PersistenceElement::writeBytes(
        os,
        ((const unsigned char*)(line4.toCString())),
        ((int)(strlen(line4.toCString()))));

    // One row at a time, without allocating a pixel per sample
    const int xSize = img->getXSize();
    unsigned char* row = new unsigned char[xSize > 0 ? 3 * xSize : 1];
    if (row == NULL) {
        // new without exceptions (Turbo C++)
        return FALSE;
    }
    RGBPixel pixel;
    int x;
    int y;
    for (y = 0; y < img->getYSize(); y++) {
        for (x = 0; x < xSize; x++) {
            img->getPixelRgb(x, y, &pixel);
            row[3 * x] = (unsigned char)pixel.r;
            row[3 * x + 1] = (unsigned char)pixel.g;
            row[3 * x + 2] = (unsigned char)pixel.b;
        }
        PersistenceElement::writeBytes(os, row, 3 * xSize);
    }
    delete[] row;

    return TRUE;
}

BOOLEAN ImagePersistence::exportRGBE(const File& fd, const RGBImageFloatUncompressed* img) {
    if (img == NULL) {
        return FALSE;
    }

    String rgbeExportName = fd.getPath();
    FileOutputStream fos(rgbeExportName.toCString());
    BufferedOutputStream writer(&fos);
    return exportRGBE(writer, img);
}

BOOLEAN ImagePersistence::exportRGBE(OutputStream& os, const RGBImageFloatUncompressed* img) {
    return ImagePersistenceRgbe::exportRgbe(os, img);
}

RGBImageFloatUncompressed* ImagePersistence::importRGBE(const File& inImageFd) {
    String rgbeImportName = inImageFd.getPath();
    if (!inImageFd.exists()) {
        Logger::reportMessage("ImagePersistence", Logger::ERROR, "importRGBE",
            String("Cannot open RGBE file ") + rgbeImportName);
        return NULL;
    }
    FileInputStream fis(rgbeImportName.toCString());
    BufferedInputStream reader(&fis);
    RGBImageFloatUncompressed* image = ImagePersistenceRgbe::importRgbe(reader);
    if (image == NULL) {
        Logger::reportMessage("ImagePersistence", Logger::ERROR, "importRGBE",
            String("Not a readable RGBE image: ") + rgbeImportName);
    }
    return image;
}


BOOLEAN ImagePersistence::exportJPEG(const File& fd, Image* img, int quality) {
#ifndef VITRAL_WITH_JPEG
    (void)fd;
    (void)img;
    (void)quality;
    fprintf(stderr, "ERROR: ImagePersistence: JPEG support not compiled in.\n");
    fprintf(stderr, "       Recompile with -DWITH_JPEG=ON to enable JPEG support.\n");
    return FALSE;
#else
    if (img == NULL) {
        return FALSE;
    }

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE* outfile = NULL;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    String jpegExportName = fd.getPath();
    const char* filename = jpegExportName.toCString();
    const String platformName = File::toPlatformName(filename);
    if ((outfile = fopen(platformName.toCString(), "wb")) == NULL) {
        fprintf(stderr, "Cannot open file %s for writing\n", filename);
        return FALSE;
    }

    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = img->getXSize();
    cinfo.image_height = img->getYSize();
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    if (quality < 0) quality = 0;
    if (quality > 100) quality = 100;
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer;
    unsigned char* row_buffer = new unsigned char[img->getXSize() * 3];
    if (row_buffer == NULL) {
        fprintf(stderr, "Failed to allocate row buffer\n");
        fclose(outfile);
        jpeg_destroy_compress(&cinfo);
        return FALSE;
    }

    for (int y = 0; y < img->getYSize(); y++) {
        for (int x = 0; x < img->getXSize(); x++) {
            RGBPixel* p = img->getPixelRgb(x, y);
            if (p == NULL) {
                fprintf(stderr, "Failed to get pixel at (%d, %d)\n", x, y);
                delete[] row_buffer;
                fclose(outfile);
                jpeg_destroy_compress(&cinfo);
                return FALSE;
            }
            row_buffer[x * 3 + 0] = (unsigned char)p->r;
            row_buffer[x * 3 + 1] = (unsigned char)p->g;
            row_buffer[x * 3 + 2] = (unsigned char)p->b;
            delete p;
        }
        row_pointer = row_buffer;
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }

    delete[] row_buffer;

    jpeg_finish_compress(&cinfo);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);

    return TRUE;
#endif
}

BOOLEAN ImagePersistence::exportPNG(const File& fd, Image* img) {
#ifndef VITRAL_WITH_PNG
    (void)fd;
    (void)img;
    fprintf(stderr, "ERROR: ImagePersistence: PNG support not compiled in.\n");
    fprintf(stderr, "       Recompile with -DWITH_PNG=ON to enable PNG support.\n");
    return FALSE;
#else
    if (img == NULL) {
        return FALSE;
    }

    String pngExportName = fd.getPath();
    File file(pngExportName.toCString());
    if (file.isDirectory() || !file.canWrite()) {
        fprintf(stderr, "Cannot open file %s for writing\n", pngExportName.toCString());
        return FALSE;
    }
    FileOutputStream fos(pngExportName.toCString());
    BufferedOutputStream writer(&fos);
    return exportPNG(writer, img);
#endif
}

#ifdef VITRAL_WITH_PNG
static void writePngData(png_structp pngPtr, png_bytep data, png_size_t length) {
    OutputStream* os = (OutputStream*)png_get_io_ptr(pngPtr);
    PersistenceElement::writeBytes(*os, data, (int)length);
}

static void flushPngData(png_structp pngPtr) {
    OutputStream* os = (OutputStream*)png_get_io_ptr(pngPtr);
    os->flush();
}
#endif

BOOLEAN ImagePersistence::exportPNG(OutputStream& os, Image* img) {
#ifndef VITRAL_WITH_PNG
    (void)os;
    (void)img;
    fprintf(stderr, "ERROR: ImagePersistence: PNG support not compiled in.\n");
    fprintf(stderr, "       Recompile with -DWITH_PNG=ON to enable PNG support.\n");
    return FALSE;
#else
    if (img == NULL || img->getXSize() <= 0 || img->getYSize() <= 0) {
        return FALSE;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        return FALSE;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        return FALSE;
    }

    // One row at a time, without allocating a pixel per sample
    const int xSize = img->getXSize();
    unsigned char* row = new unsigned char[3 * xSize];
    if (row == NULL) {
        // new without exceptions (Turbo C++)
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return FALSE;
    }

    // libpng reports its errors by jumping back here
    if (setjmp(png_jmpbuf(png_ptr))) {
        delete[] row;
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return FALSE;
    }

    png_set_write_fn(png_ptr, &os, writePngData, flushPngData);

    png_set_IHDR(png_ptr, info_ptr,
                 img->getXSize(), img->getYSize(),
                 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    RGBPixel pixel;
    int x;
    int y;
    for (y = 0; y < img->getYSize(); y++) {
        for (x = 0; x < xSize; x++) {
            img->getPixelRgb(x, y, &pixel);
            row[x * 3 + 0] = (unsigned char)pixel.r;
            row[x * 3 + 1] = (unsigned char)pixel.g;
            row[x * 3 + 2] = (unsigned char)pixel.b;
        }
        png_write_row(png_ptr, row);
    }

    png_write_end(png_ptr, NULL);

    delete[] row;
    png_destroy_write_struct(&png_ptr, &info_ptr);

    return TRUE;
#endif
}
