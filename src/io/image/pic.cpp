#include "io/image/dkcolor.h"
#include "io/image/pic.h"
#include "java/io/File.h"
#include "java/lang/System.h"

PicOutputHandle::PicOutputHandle(const char *filename, int w, int h) {
    ImageOutputHandle::init("high dynamic range PIC", w, h);

    fileDescriptor = java::io::File::openHandle(filename, "wb");
    if ( fileDescriptor == nullptr ) {
        java::lang::System::err.printf("Can't open PIC output");
        return;
    }

    writeHeader();
}

PicOutputHandle::~PicOutputHandle() {
    if ( fileDescriptor != nullptr ) {
        java::io::File::closeHandle(fileDescriptor);
    }
    fileDescriptor = nullptr;
}

/**
Writes scanline of high-dynamic range radiance data in RGB format
*/
int
PicOutputHandle::writeRadianceRGB(ColorRgb *rgbRadiance) {
    int result = 0;

    if ( fileDescriptor != nullptr ) {
        result = dkColorWriteScan(reinterpret_cast<DK_COLOR *>(rgbRadiance), width, fileDescriptor);
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
    fprintf(fileDescriptor, "#?RADIANCE\n");
    fprintf(fileDescriptor, "#RPK PicOutputHandler (compiled %s)\n", __DATE__);
    fprintf(fileDescriptor, "FORMAT=32-bit_rle_rgbe\n");
    fprintf(fileDescriptor, "\n");
    fprintf(fileDescriptor, "-Y %d +X %d\n", height, width);
}
