#include "io/image/ppm.h"

PPMOutputHandle::PPMOutputHandle(FILE *_fp, int w, int h) {
    ImageOutputHandle::init("PPM", w, h);
    fp = _fp;

    if ( fp ) {
        fprintf(fp, "P6\n%d %d\n255\n", width, height);
    }
}

int
PPMOutputHandle::writeDisplayRGB(unsigned char *rgb) {
    return fp ? (int)fwrite(rgb, 3, width, fp) : 0;
}
