#include "IMAGE/pic.h"
// this file defines COLOR; may interfere with rpk's COLOR!
#include "IMAGE/dkcolor.h"

PicOutputHandle::
PicOutputHandle(char *filename, int w, int h) {
    ImageOutputHandle::init("high dynamic range PIC", w, h);

    if ((pic = fopen(filename, "wb")) == nullptr ) {
        fprintf(stderr, "Can't open PIC output");
        return;
    }

    writeHeader();
}

PicOutputHandle::
~PicOutputHandle() {
    if ( pic ) {
        fclose(pic);
    }
    pic = nullptr;
}

// writes scanline of high-dynamic range radiance data in RGB format
int PicOutputHandle::
WriteRadianceRGB(float *rgbrad) {
    int result = 0;

    if ( pic ) {
        result = fwritescan((COLOR *) rgbrad, width, pic);
    }

    if ( result ) {
        return width;
    } else {
        return 0;
    } // We don't know how many pixels were actually written
}

void PicOutputHandle::
writeHeader() {
    // simple RADIANCE header
    fprintf(pic, "#?RADIANCE\n");
    fprintf(pic, "#RPK PicOutputHandler (compiled %s)\n", __DATE__);
    fprintf(pic, "FORMAT=32-bit_rle_rgbe\n");
    fprintf(pic, "\n");
    fprintf(pic, "-Y %d +X %d\n", height, width);
}
