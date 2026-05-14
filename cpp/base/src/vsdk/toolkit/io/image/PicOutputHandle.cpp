#include "vsdk/toolkit/java/io/FileOutputStream.h"
#include "vsdk/toolkit/java/lang/System.h"
#include "vsdk/toolkit/java/util/Formatter.h"
#include "vsdk/toolkit/io/wrapper/PersistenceElement.h"
#include "vsdk/toolkit/io/image/Dkcolor.h"
#include "vsdk/toolkit/io/image/PicOutputHandle.h"

java::String
PicOutputHandle::formatToString(const char *format, va_list arguments) {
    if ( format == nullptr ) {
        return java::String();
    }

    char localBuffer[256];
    va_list argumentsCopy;
    va_copy(argumentsCopy, arguments);
    const int required = java::Formatter::vformat(localBuffer, static_cast<int>(sizeof(localBuffer)), format, argumentsCopy);
    va_end(argumentsCopy);

    if ( required < 0 ) {
        return java::String();
    }
    if ( required < static_cast<int>(sizeof(localBuffer)) ) {
        return java::String(localBuffer);
    }

    char *dynamicBuffer = new char[required + 1];
    va_copy(argumentsCopy, arguments);
    java::Formatter::vformat(dynamicBuffer, required + 1, format, argumentsCopy);
    va_end(argumentsCopy);

    java::String result(dynamicBuffer);
    delete[] dynamicBuffer;
    return result;
}

void
PicOutputHandle::writeFormatted(java::OutputStream *outputStream, const char *format, ...) {
    if ( outputStream == nullptr || format == nullptr ) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    java::String text = formatToString(format, arguments);
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

    java::File file(filename);
    if ( !file.canWrite() || file.isDirectory() ) {
        outputStream = nullptr;
        java::System::err.printf("Can't open PIC output");
        return;
    }
    outputStream = new java::FileOutputStream(filename);

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
PicOutputHandle::writeRadianceRGB(ColorRgbMutable *rgbRadiance) {
    int result = 0;

    if ( outputStream != nullptr ) {
        result = DkColor::writeScan(reinterpret_cast<DK_COLOR *>(rgbRadiance), width, outputStream);
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
