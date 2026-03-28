#include "io/image/ppm.h"
#include "io/PersistenceElement.h"

PPMOutputHandle::PPMOutputHandle(FILE *_fp, int w, int h) {
    ImageOutputHandle::init("PPM", w, h);
    fp = _fp;
    outputStream = nullptr;

    if ( fp ) {
        fprintf(fp, "P6\n%d %d\n255\n", width, height);
    }
}

PPMOutputHandle::PPMOutputHandle(java::io::OutputStream *_outputStream, int w, int h) {
    ImageOutputHandle::init("PPM", w, h);
    fp = nullptr;
    outputStream = _outputStream;

    if ( outputStream != nullptr ) {
        char header[64];
        const int headerLength = std::snprintf(header, sizeof(header), "P6\n%d %d\n255\n", width, height);
        if ( headerLength > 0 ) {
            vsdk::PersistenceElement::writeBytes(
                *outputStream,
                reinterpret_cast<const unsigned char *>(header),
                headerLength);
        }
    }
}

int
PPMOutputHandle::writeDisplayRGB(unsigned char *rgb) {
    if ( fp != nullptr ) {
        return static_cast<int>(fwrite(rgb, 3, width, fp));
    }
    if ( outputStream != nullptr ) {
        vsdk::PersistenceElement::writeBytes(*outputStream, rgb, 3 * width);
        return width;
    }
    return 0;
}
