#include "io/image/dkcolor.h"
#include "io/image/pic.h"

PicOutputHandle::PicOutputHandle(const char *filename, int w, int h) {
    ImageOutputHandle::init("high dynamic range PIC", w, h);

    pic = fopen(filename, "wb");
    if ( pic == nullptr ) {
        fprintf(stderr, "Can't open PIC output");
        return;
    }

    writeHeader();
}

PicOutputHandle::~PicOutputHandle() {
    if ( pic != nullptr ) {
        fclose(pic);
    }
    pic = nullptr;
}

/**
Writes scanline of high-dynamic range radiance data in RGB format
*/
int
PicOutputHandle::writeRadianceRGB(ColorRgb *rgbRadiance) {
    int result = 0;

    if ( pic != nullptr ) {
        result = dkColorWriteScan((COLOR *)rgbRadiance, width, pic);
        dkColorFreeBuffer();
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
    fprintf(pic, "#?RADIANCE\n");
    fprintf(pic, "#RPK PicOutputHandler (compiled %s)\n", __DATE__);
    fprintf(pic, "FORMAT=32-bit_rle_rgbe\n");
    fprintf(pic, "\n");
    fprintf(pic, "-Y %d +X %d\n", height, width);
}
