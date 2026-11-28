#include "java/util/Collections.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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
    if (std::strncmp(fourCC, "DXT1", 4) == 0) {
        return RGBAImageCompressed::COMPRESSION_DXT1;
    }
    if (std::strncmp(fourCC, "DXT3", 4) == 0) {
        return RGBAImageCompressed::COMPRESSION_DXT3;
    }
    if (std::strncmp(fourCC, "DXT5", 4) == 0) {
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

IndexedColorImageUncompressed* importSgiIndexed(const java::File& inImageFd)
{
    java::String filePath = inImageFd.getPath();
    FILE* f = std::fopen(filePath.toCString(), "rb");
    if (f == nullptr) return nullptr;

    unsigned char header[512];
    if (std::fread(header, 1, sizeof(header), f) != sizeof(header)) {
        std::fclose(f);
        return nullptr;
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
        std::fclose(f);
        return nullptr;
    }

    const int tables = (int)ySize * (int)channels;
    java::ArrayList<unsigned int> starts;
    starts.reserve((long int)tables);
    for (long int i = 0; i < (long int)tables; i++) starts.add(0u);
    java::ArrayList<unsigned int> lengths;
    lengths.reserve((long int)tables);
    for (long int i = 0; i < (long int)tables; i++) lengths.add(0u);
    java::ArrayList<unsigned char> tmp;
    tmp.reserve((long int)tables * 4);
    for (long int i = 0; i < (long int)tables * 4; i++) tmp.add(0u);
    if (std::fread(tmp.data(), 1, (size_t)tmp.size(), f) != (size_t)tmp.size()) {
        std::fclose(f);
        return nullptr;
    }
    for (int i = 0; i < tables; i++) starts[(long int)i] = readU32BE(&tmp[(long int)i * 4]);
    if (std::fread(tmp.data(), 1, (size_t)tmp.size(), f) != (size_t)tmp.size()) {
        std::fclose(f);
        return nullptr;
    }
    for (int i = 0; i < tables; i++) lengths[(long int)i] = readU32BE(&tmp[(long int)i * 4]);

    IndexedColorImageUncompressed* img = new IndexedColorImageUncompressed(new GrayScalePalette());
    if (!img->init((int)xSize, (int)ySize)) {
        delete img;
        std::fclose(f);
        return nullptr;
    }

    for (int y = 0; y < (int)ySize; y++) {
        const unsigned int start = starts[(long int)y];
        const unsigned int length = lengths[(long int)y];
        if (length == 0) continue;
        if (std::fseek(f, (long)start, SEEK_SET) != 0) continue;
        java::ArrayList<unsigned char> line;
        line.reserve((long int)length);
        for (long int li = 0; li < (long int)length; li++) line.add(0u);
        if (std::fread(line.data(), 1, length, f) != length) continue;

        int x = 0;
        for (unsigned int pos = 0; pos < length && x < (int)xSize; pos++) {
            bool literal = (line[(long int)pos] & 0x80) != 0;
            int count = (line[(long int)pos] & 0x7F);
            if (count == 0) break;

            if (literal) {
                for (int i = 0; i < count && x < (int)xSize; i++) {
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
                for (int i = 0; i < count && x < (int)xSize; i++) {
                    img->putPixel(x, (int)ySize - y - 1, v);
                    x++;
                }
            }
        }
    }

    std::fclose(f);
    return img;
}

RGBAImageCompressed* importDDSCompressed(const java::File& inImageFd) {
    java::String filePath = inImageFd.getPath();
    FILE* f = std::fopen(filePath.toCString(), "rb");
    if (f == nullptr) {
        std::fprintf(stderr, "Error: could not open DDS file \"%s\"\\n", filePath.toCString());
        return nullptr;
    }

    std::fseek(f, 0, SEEK_END);
    long fileSize = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (fileSize < 128) {
        std::fclose(f);
        std::fprintf(stderr, "Error: DDS file too short\\n");
        return nullptr;
    }

    java::ArrayList<unsigned char> fileData;
    fileData.reserve((long int)fileSize);
    for (long int i = 0; i < (long int)fileSize; i++) fileData.add(0u);
    size_t readCount = std::fread(fileData.data(), 1, (size_t)fileData.size(), f);
    std::fclose(f);
    if (readCount != (size_t)fileData.size()) {
        std::fprintf(stderr, "Error: could not read complete DDS file\\n");
        return nullptr;
    }

    if (fileData[0] != 'D' || fileData[1] != 'D' ||
        fileData[2] != 'S' || fileData[3] != (unsigned char)' ') {
        std::fprintf(stderr, "Error: DDS signature not recognized\\n");
        return nullptr;
    }

    int headerSize = readIntLE(fileData.data(), 4);
    if (headerSize != 124) {
        std::fprintf(stderr, "Error: DDS header size not recognized\\n");
        return nullptr;
    }

    int height = readIntLE(fileData.data(), 12);
    int width = readIntLE(fileData.data(), 16);
    int pixelFormatFlags = readIntLE(fileData.data(), 80);
    char fourCC[5] = {0, 0, 0, 0, 0};
    std::memcpy(fourCC, fileData.data() + 84, 4);

    if ((pixelFormatFlags & 0x04) == 0) {
        std::fprintf(stderr, "Error: DDS file does not use a FourCC compressed format\\n");
        return nullptr;
    }

    int compressionFormat = ddsFourCCToCompressionFormat(fourCC);
    if (compressionFormat == RGBAImageCompressed::COMPRESSION_UNKNOWN) {
        std::fprintf(stderr, "Error: DDS compressed format not supported: %.4s\\n", fourCC);
        return nullptr;
    }

    int dataOffset = 128;
    int dataSize = static_cast<int>(fileData.size()) - dataOffset;
    if (dataSize <= 0) {
        std::fprintf(stderr, "Error: DDS has no payload data\\n");
        return nullptr;
    }

    RGBAImageCompressed* image = new RGBAImageCompressed();
    if (!image->initCompressed(width, height, compressionFormat,
                               reinterpret_cast<char*>(fileData.data() + dataOffset),
                               dataSize)) {
        delete image;
        std::fprintf(stderr, "Error: could not initialize compressed DDS image\\n");
        return nullptr;
    }

    return image;
}

#ifdef VITRAL_WITH_JPEG
#include <jpeglib.h>
#endif

#ifdef VITRAL_WITH_PNG
#include <png.h>
#endif

bool ImagePersistence::isTextComment(const java::String& line) {
    if (line.isEmpty()) {
        return false;
    }

    int i = 0;
    while (i < line.length() &&
           (line.charAt(i) != ' ' && line.charAt(i) != '\t')) {
        i++;
    }

    return i < line.length() && line.charAt(i) == '#';
}

java::String* ImagePersistence::extractExtensionFromFile(const java::File& fd) {
    java::String path = fd.getName();
    const char* pathStr = path.toCString();

    const char* lastDot = strrchr(pathStr, '.');
    if (lastDot != nullptr && lastDot != pathStr) {
        java::String* ext = new java::String(lastDot + 1);
        char extBuffer[256];
        strcpy(extBuffer, ext->toCString());
        for (int i = 0; i < (int)strlen(extBuffer); i++) {
            extBuffer[i] = tolower(extBuffer[i]);
        }
        delete ext;
        java::String* result = new java::String(extBuffer);
        return result;
    }
    return new java::String("");
}

RGBImageUncompressed* ImagePersistence::importRGB(const java::File& inImageFd) {
    java::String* type = extractExtensionFromFile(inImageFd);
    RGBImageUncompressed* retImage = nullptr;

#ifdef VITRAL_WITH_JPEG
    if (type->equals("jpg") || type->equals("jpeg")) {
        java::String nameStr = inImageFd.getPath();
        const char* filename = nameStr.toCString();
        FILE* infile = fopen(filename, "rb");
        if (infile == nullptr) {
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
        java::String pngNameStr = inImageFd.getPath();
        const char* filename = pngNameStr.toCString();
        FILE* fp = std::fopen(filename, "rb");
        if (fp == nullptr) {
            delete type;
            return new RGBImageUncompressed();
        }

        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png_ptr) {
            std::fclose(fp);
            delete type;
            return new RGBImageUncompressed();
        }
        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
            png_destroy_read_struct(&png_ptr, nullptr, nullptr);
            std::fclose(fp);
            delete type;
            return new RGBImageUncompressed();
        }

        if (setjmp(png_jmpbuf(png_ptr))) {
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
            std::fclose(fp);
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

        java::ArrayList<unsigned char> pixels;
        pixels.reserve((long int)width * (long int)height * 3);
        for (long int i = 0; i < (long int)width * (long int)height * 3; i++) pixels.add(0u);
        java::ArrayList<png_bytep> row_ptrs;
        row_ptrs.reserve((long int)height);
        for (long int i = 0; i < (long int)height; i++) row_ptrs.add(nullptr);
        for (int y = 0; y < height; y++) {
            row_ptrs[(long int)y] = reinterpret_cast<png_bytep>(&pixels[(long int)y * (long int)width * 3]);
        }
        png_read_image(png_ptr, row_ptrs.data());

        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        std::fclose(fp);

        retImage = new RGBImageUncompressed();
        if (!retImage->initNoFill(width, height)) {
            delete retImage;
            delete type;
            return new RGBImageUncompressed();
        }

        long int pos = 0;
        for (int y = 0; y < height; y++) {
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
        java::String ppmNameStr = inImageFd.getPath();
        java::FileInputStream fis(ppmNameStr.toCString());
        java::BufferedInputStream bis(&fis);

        bool exit = false;
        char* lineStr = nullptr;
        int stage = 1;
        int xSize = 0, ySize = 0;
        int i = 0;

        do {
            if (lineStr != nullptr) {
                delete[] lineStr;
            }
            lineStr = PersistenceElement::readAsciiLine(bis);

            if (lineStr == nullptr) {
                break;
            }

            if (strcmp(lineStr, "255") == 0) {
                exit = true;
            }

            if (isTextComment(java::String(lineStr))) {
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
                        if (token != nullptr) {
                            xSize = atoi(token);
                            token = strtok(nullptr, " \t");
                            if (token != nullptr) {
                                ySize = atoi(token);
                            }
                        }
                        stage++;
                    }
                    break;
            }
        } while (!exit);

        if (lineStr != nullptr) {
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
                if (ppa != nullptr && ppb != nullptr) {
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

IndexedColorImageUncompressed* ImagePersistence::importIndexedColor(const java::File& inImageFd)
{
    java::String* type = extractExtensionFromFile(inImageFd);
    if (type->equals("bw") || type->equals("sgi")) {
        IndexedColorImageUncompressed* sgi = importSgiIndexed(inImageFd);
        delete type;
        if (sgi != nullptr) {
            return sgi;
        }
        return new IndexedColorImageUncompressed(new GrayScalePalette());
    }
    delete type;

    RGBImageUncompressed* rgb = importRGB(inImageFd);
    IndexedColorImageUncompressed* indexed = new IndexedColorImageUncompressed();
    if (rgb == nullptr || rgb->getXSize() <= 0 || rgb->getYSize() <= 0) {
        if (rgb != nullptr) delete rgb;
        return indexed;
    }

    indexed->init(rgb->getXSize(), rgb->getYSize());
    for (int y = 0; y < rgb->getYSize(); y++) {
        for (int x = 0; x < rgb->getXSize(); x++) {
            RGBPixel* p = rgb->getPixelRgb(x, y);
            if (p != nullptr) {
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

RGBAImageUncompressed* ImagePersistence::importRGBA(const java::File& inImageFd) {
#ifdef VITRAL_WITH_PNG
    java::String* type = extractExtensionFromFile(inImageFd);
    bool isPng = type->equals("png");
    delete type;
    if (isPng) {
        java::String pngNameStr = inImageFd.getPath();
        FILE* fp = std::fopen(pngNameStr.toCString(), "rb");
        if (!fp) {
            fprintf(stderr, "Cannot open PNG file: %s\n", pngNameStr.toCString());
            return nullptr;
        }
        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        png_infop info_ptr = png_ptr ? png_create_info_struct(png_ptr) : nullptr;
        if (!png_ptr || !info_ptr) {
            png_destroy_read_struct(&png_ptr, nullptr, nullptr);
            std::fclose(fp);
            return nullptr;
        }
        if (setjmp(png_jmpbuf(png_ptr))) {
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
            std::fclose(fp);
            return nullptr;
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

        java::ArrayList<unsigned char> pixels = java::Collections::nCopies<unsigned char>((size_t)width * (size_t)height * 4, (unsigned char)0);
        java::ArrayList<png_bytep> rows = java::Collections::nCopies<png_bytep>((size_t)height, nullptr);
        for (int y = 0; y < height; y++) {
            rows[(size_t)y] = &pixels[(size_t)y * (size_t)width * 4];
        }
        png_read_image(png_ptr, rows.data());
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        std::fclose(fp);

        RGBAImageUncompressed* retImage = new RGBAImageUncompressed();
        if (!retImage->init(width, height)) {
            delete retImage;
            return nullptr;
        }
        size_t pos = 0;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                retImage->putPixel(x, y, (char)pixels[pos], (char)pixels[pos + 1],
                                   (char)pixels[pos + 2], (char)pixels[pos + 3]);
                pos += 4;
            }
        }
        return retImage;
    }
#endif

    // Formats without transparency: an opaque image
    RGBImageUncompressed* rgb = importRGB(inImageFd);
    if (rgb == nullptr) {
        return nullptr;
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

Image* ImagePersistence::importImage(const java::File& inImageFd) {
    java::String* type = extractExtensionFromFile(inImageFd);
    if (type->equals("dds")) {
        RGBAImageCompressed* ddsImage = importDDSCompressed(inImageFd);
        delete type;
        return ddsImage;
    }

    delete type;
    return importRGB(inImageFd);
}

bool ImagePersistence::exportPPM(const java::File& fd, Image* img) {
    if (img == nullptr) {
        return false;
    }

    java::String ppmExportName = fd.getPath();
    java::FileOutputStream fos(ppmExportName.toCString());
    java::BufferedOutputStream writer(&fos);
    return exportPPM(writer, img);
}

bool ImagePersistence::exportPPM(java::OutputStream& os, Image* img) {
    if (img == nullptr) {
        return false;
    }

    java::String line1("P6\n");
    java::String line2("# Image generated by VitralSDK (https://github.com/OscarChavarro/vitral)\n");

    char sizeStr[256];
    snprintf(sizeStr, sizeof(sizeStr), "%d %d\n", img->getXSize(), img->getYSize());
    java::String line3(sizeStr);

    java::String line4("255\n");

    // Use raw ASCII writes (without trailing NUL bytes) for valid PPM header text lines.
    PersistenceElement::writeBytes(
        os,
        reinterpret_cast<const unsigned char*>(line1.toCString()),
        static_cast<int>(std::strlen(line1.toCString())));
    PersistenceElement::writeBytes(
        os,
        reinterpret_cast<const unsigned char*>(line2.toCString()),
        static_cast<int>(std::strlen(line2.toCString())));
    PersistenceElement::writeBytes(
        os,
        reinterpret_cast<const unsigned char*>(line3.toCString()),
        static_cast<int>(std::strlen(line3.toCString())));
    PersistenceElement::writeBytes(
        os,
        reinterpret_cast<const unsigned char*>(line4.toCString()),
        static_cast<int>(std::strlen(line4.toCString())));

    // One row at a time, without allocating a pixel per sample
    const int xSize = img->getXSize();
    unsigned char* row = new unsigned char[xSize > 0 ? 3 * xSize : 1];
    RGBPixel pixel;
    for (int y = 0; y < img->getYSize(); y++) {
        for (int x = 0; x < xSize; x++) {
            img->getPixelRgb(x, y, &pixel);
            row[3 * x] = (unsigned char)pixel.r;
            row[3 * x + 1] = (unsigned char)pixel.g;
            row[3 * x + 2] = (unsigned char)pixel.b;
        }
        PersistenceElement::writeBytes(os, row, 3 * xSize);
    }
    delete[] row;

    return true;
}

bool ImagePersistence::exportRGBE(const java::File& fd, const RGBImageFloatUncompressed* img) {
    if (img == nullptr) {
        return false;
    }

    java::String rgbeExportName = fd.getPath();
    java::FileOutputStream fos(rgbeExportName.toCString());
    java::BufferedOutputStream writer(&fos);
    return exportRGBE(writer, img);
}

bool ImagePersistence::exportRGBE(java::OutputStream& os, const RGBImageFloatUncompressed* img) {
    return ImagePersistenceRgbe::exportRgbe(os, img);
}

RGBImageFloatUncompressed* ImagePersistence::importRGBE(const java::File& inImageFd) {
    java::String rgbeImportName = inImageFd.getPath();
    if (!inImageFd.exists()) {
        Logger::reportMessage("ImagePersistence", Logger::ERROR, "importRGBE",
            java::String("Cannot open RGBE file ") + rgbeImportName);
        return nullptr;
    }
    java::FileInputStream fis(rgbeImportName.toCString());
    java::BufferedInputStream reader(&fis);
    RGBImageFloatUncompressed* image = ImagePersistenceRgbe::importRgbe(reader);
    if (image == nullptr) {
        Logger::reportMessage("ImagePersistence", Logger::ERROR, "importRGBE",
            java::String("Not a readable RGBE image: ") + rgbeImportName);
    }
    return image;
}

bool ImagePersistence::exportJPEG(const java::File& fd, Image* img, int quality) {
#ifndef VITRAL_WITH_JPEG
    (void)fd;
    (void)img;
    (void)quality;
    fprintf(stderr, "ERROR: ImagePersistence: JPEG support not compiled in.\n");
    fprintf(stderr, "       Recompile with -DWITH_JPEG=ON to enable JPEG support.\n");
    return false;
#else
    if (img == nullptr) {
        return false;
    }

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE* outfile = nullptr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    java::String jpegExportName = fd.getPath();
    const char* filename = jpegExportName.toCString();
    if ((outfile = fopen(filename, "wb")) == nullptr) {
        fprintf(stderr, "Cannot open file %s for writing\n", filename);
        return false;
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
    if (row_buffer == nullptr) {
        fprintf(stderr, "Failed to allocate row buffer\n");
        fclose(outfile);
        jpeg_destroy_compress(&cinfo);
        return false;
    }

    for (int y = 0; y < img->getYSize(); y++) {
        for (int x = 0; x < img->getXSize(); x++) {
            RGBPixel* p = img->getPixelRgb(x, y);
            if (p == nullptr) {
                fprintf(stderr, "Failed to get pixel at (%d, %d)\n", x, y);
                delete[] row_buffer;
                fclose(outfile);
                jpeg_destroy_compress(&cinfo);
                return false;
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

    return true;
#endif
}

bool ImagePersistence::exportPNG(const java::File& fd, Image* img) {
#if defined(VITRAL_PNG_READ_ONLY)
    (void)fd;
    (void)img;
    return false;
#elif !defined(VITRAL_WITH_PNG)
    (void)fd;
    (void)img;
    fprintf(stderr, "ERROR: ImagePersistence: PNG support not compiled in.\n");
    fprintf(stderr, "       Recompile with -DWITH_PNG=ON to enable PNG support.\n");
    return false;
#else
    if (img == nullptr) {
        return false;
    }

    java::String pngExportName = fd.getPath();
    java::File file(pngExportName.toCString());
    if (file.isDirectory() || !file.canWrite()) {
        fprintf(stderr, "Cannot open file %s for writing\n", pngExportName.toCString());
        return false;
    }
    java::FileOutputStream fos(pngExportName.toCString());
    java::BufferedOutputStream writer(&fos);
    return exportPNG(writer, img);
#endif
}

#if defined(VITRAL_WITH_PNG) && !defined(VITRAL_PNG_READ_ONLY)
namespace {

void writePngData(png_structp pngPtr, png_bytep data, png_size_t length) {
    java::OutputStream* os = static_cast<java::OutputStream*>(png_get_io_ptr(pngPtr));
    PersistenceElement::writeBytes(*os, data, static_cast<int>(length));
}

void flushPngData(png_structp pngPtr) {
    java::OutputStream* os = static_cast<java::OutputStream*>(png_get_io_ptr(pngPtr));
    os->flush();
}

}
#endif

bool ImagePersistence::exportPNG(java::OutputStream& os, Image* img) {
#if defined(VITRAL_PNG_READ_ONLY)
    (void)os;
    (void)img;
    return false;
#elif !defined(VITRAL_WITH_PNG)
    (void)os;
    (void)img;
    fprintf(stderr, "ERROR: ImagePersistence: PNG support not compiled in.\n");
    fprintf(stderr, "       Recompile with -DWITH_PNG=ON to enable PNG support.\n");
    return false;
#else
    if (img == nullptr || img->getXSize() <= 0 || img->getYSize() <= 0) {
        return false;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr) {
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, nullptr);
        return false;
    }

    // One row at a time, without allocating a pixel per sample
    const int xSize = img->getXSize();
    unsigned char* row = new unsigned char[3 * xSize];

    // libpng reports its errors by jumping back here
    if (setjmp(png_jmpbuf(png_ptr))) {
        delete[] row;
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
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
    for (int y = 0; y < img->getYSize(); y++) {
        for (int x = 0; x < xSize; x++) {
            img->getPixelRgb(x, y, &pixel);
            row[x * 3 + 0] = (unsigned char)pixel.r;
            row[x * 3 + 1] = (unsigned char)pixel.g;
            row[x * 3 + 2] = (unsigned char)pixel.b;
        }
        png_write_row(png_ptr, row);
    }

    png_write_end(png_ptr, nullptr);

    delete[] row;
    png_destroy_write_struct(&png_ptr, &info_ptr);

    return true;
#endif
}
