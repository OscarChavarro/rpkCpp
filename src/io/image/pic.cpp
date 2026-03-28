#include <cstdarg>

#include "io/image/dkcolor.h"
#include "io/image/pic.h"
#include "io/PersistenceElement.h"
#include "java/io/FileOutputStream.h"
#include "java/lang/System.h"

static void
picWriteFormatted(java::io::OutputStream *outputStream, const char *format, ...) {
    if ( outputStream == nullptr || format == nullptr ) {
        return;
    }

    char localBuffer[256];
    va_list arguments;
    va_start(arguments, format);
    const int required = std::vsnprintf(localBuffer, sizeof(localBuffer), format, arguments);
    va_end(arguments);

    if ( required <= 0 ) {
        return;
    }

    if ( required < static_cast<int>(sizeof(localBuffer)) ) {
        vsdk::PersistenceElement::writeBytes(
            *outputStream,
            reinterpret_cast<const unsigned char *>(localBuffer),
            required);
        return;
    }

    char *dynamicBuffer = new char[required + 1];
    va_start(arguments, format);
    std::vsnprintf(dynamicBuffer, required + 1, format, arguments);
    va_end(arguments);
    vsdk::PersistenceElement::writeBytes(
        *outputStream,
        reinterpret_cast<const unsigned char *>(dynamicBuffer),
        required);
    delete[] dynamicBuffer;
}

PicOutputHandle::PicOutputHandle(const char *filename, int w, int h) {
    ImageOutputHandle::init("high dynamic range PIC", w, h);

    java::io::FileOutputStream *fileStream = new java::io::FileOutputStream(filename);
    if ( !fileStream->isOpen() ) {
        delete fileStream;
        outputStream = nullptr;
        java::lang::System::err.printf("Can't open PIC output");
        return;
    }
    outputStream = fileStream;

    writeHeader();
}

PicOutputHandle::~PicOutputHandle() {
    if ( outputStream != nullptr ) {
        outputStream->close();
        delete outputStream;
    }
    outputStream = nullptr;
}

/**
Writes scanline of high-dynamic range radiance data in RGB format
*/
int
PicOutputHandle::writeRadianceRGB(ColorRgb *rgbRadiance) {
    int result = 0;

    if ( outputStream != nullptr ) {
        result = dkColorWriteScan(reinterpret_cast<DK_COLOR *>(rgbRadiance), width, outputStream);
    }

    if ( result ) {
        return width;
    } else {
        // We don't know how many pixels were actually written
        return 0;
    }
}

void
PicOutputHandle::writeHeader() {
    // Simple RADIANCE header
    picWriteFormatted(outputStream, "#?RADIANCE\n");
    picWriteFormatted(outputStream, "#RPK PicOutputHandler (compiled %s)\n", __DATE__);
    picWriteFormatted(outputStream, "FORMAT=32-bit_rle_rgbe\n");
    picWriteFormatted(outputStream, "\n");
    picWriteFormatted(outputStream, "-Y %d +X %d\n", height, width);
}
