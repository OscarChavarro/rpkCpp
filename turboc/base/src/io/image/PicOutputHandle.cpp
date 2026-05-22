#include <stdarg.h>

#include "java/io/FileOutputStream.h"
#include "java/lang/System.h"
#include "java/util/Formatter.h"
#include "io/wrapper/PersistenceElement.h"
#include "io/image/Dkcolor.h"
#include "io/image/PicOutputHandle.h"

#ifndef va_copy
#if defined(__va_copy)
#define va_copy(dst, src) __va_copy((dst), (src))
#else
#define va_copy(dst, src) ((dst) = (src))
#endif
#endif

String
PicOutputHandle::formatToString(const char *format, va_list arguments) {
    if ( format == NULL ) {
        return String();
    }

    char localBuffer[256];
    va_list argumentsCopy;
    va_copy(argumentsCopy, arguments);
    const int required = Formatter::vformat(localBuffer, ((int)(sizeof(localBuffer))), format, argumentsCopy);
    va_end(argumentsCopy);

    if ( required < 0 ) {
        return String();
    }
    if ( required < ((int)(sizeof(localBuffer))) ) {
        return String(localBuffer);
    }

    char *dynamicBuffer = new char[required + 1];
    va_copy(argumentsCopy, arguments);
    Formatter::vformat(dynamicBuffer, required + 1, format, argumentsCopy);
    va_end(argumentsCopy);

    String result(dynamicBuffer);
    delete[] dynamicBuffer;
    return result;
}

void
PicOutputHandle::writeFormatted(OutputStream *outputStream, const char *format, ...) {
    if ( outputStream == NULL || format == NULL ) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    String text = formatToString(format, arguments);
    va_end(arguments);

    if ( text.isEmpty() ) {
        return;
    }
    PersistenceElement::writeBytes(
        *outputStream,
        ((const unsigned char *)(text.toCString())),
        text.length());
}

PicOutputHandle::PicOutputHandle(const char *filename, int w, int h) {
    ImageOutputHandle::init("high dynamic range PIC", w, h);

    File file(filename);
    if ( !file.canWrite() || file.isDirectory() ) {
        outputStream = NULL;
        System::err.printf("Can't open PIC output");
        return;
    }
    outputStream = new FileOutputStream(filename);

    writeHeader();
}

PicOutputHandle::~PicOutputHandle() {
    if ( outputStream != NULL ) {
        outputStream->close();
        delete outputStream;
    }
    outputStream = NULL;
}

/**
Writes scanline of high-dynamic range radiance data in RGB format
*/
int
PicOutputHandle::writeRadianceRGB(const ColorRgb *rgbRadiance) {
    int result = 0;

    if ( outputStream != NULL ) {
        float *scanline = new float[width * 3];
        for ( int i = 0; i < width; i++ ) {
            scanline[3 * i] = rgbRadiance[i].getR();
            scanline[3 * i + 1] = rgbRadiance[i].getG();
            scanline[3 * i + 2] = rgbRadiance[i].getB();
        }
        result = DkColor::writeScan(((DK_COLOR *)(scanline)), width, outputStream);
        delete[] scanline;
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
    writeFormatted(outputStream, "#?RADIANCE\n");
    writeFormatted(outputStream, "#RPK PicOutputHandler (compiled %s)\n", __DATE__);
    writeFormatted(outputStream, "FORMAT=32-bit_rle_rgbe\n");
    writeFormatted(outputStream, "\n");
    writeFormatted(outputStream, "-Y %d +X %d\n", height, width);
}
