#include "java/io/FileOutputStream.h"
#include "java/lang/System.h"
#include "java/util/Formatter.h"
#include "io/wrapper/PersistenceElement.h"
#include "io/image/Dkcolor.h"
#include "io/image/PicOutputHandle.h"

namespace {

static java::lang::String
formatToString(const char *format, va_list arguments) {
    if ( format == nullptr ) {
        return java::lang::String();
    }

    char localBuffer[256];
    va_list argumentsCopy;
    va_copy(argumentsCopy, arguments);
    const int required = java::util::Formatter::vformat(localBuffer, static_cast<int>(sizeof(localBuffer)), format, argumentsCopy);
    va_end(argumentsCopy);

    if ( required < 0 ) {
        return java::lang::String();
    }
    if ( required < static_cast<int>(sizeof(localBuffer)) ) {
        return java::lang::String(localBuffer);
    }

    char *dynamicBuffer = new char[required + 1];
    va_copy(argumentsCopy, arguments);
    java::util::Formatter::vformat(dynamicBuffer, required + 1, format, argumentsCopy);
    va_end(argumentsCopy);

    java::lang::String result(dynamicBuffer);
    delete[] dynamicBuffer;
    return result;
}

}

static void
picWriteFormatted(java::io::OutputStream *outputStream, const char *format, ...) {
    if ( outputStream == nullptr || format == nullptr ) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    java::lang::String text = formatToString(format, arguments);
    va_end(arguments);

    if ( text.isEmpty() ) {
        return;
    }
    vsdk::PersistenceElement::writeBytes(
        *outputStream,
        reinterpret_cast<const unsigned char *>(text.toCString()),
        text.length());
}

PicOutputHandle::PicOutputHandle(const char *filename, int w, int h) {
    ImageOutputHandle::init("high dynamic range PIC", w, h);

    java::io::File file(filename);
    if ( !file.canWrite() || file.isDirectory() ) {
        outputStream = nullptr;
        java::lang::System::err.printf("Can't open PIC output");
        return;
    }
    outputStream = new java::io::FileOutputStream(filename);

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
