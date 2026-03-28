#include "java/util/Formatter.h"

#include "io/image/ppm.h"
#include "io/wrapper/PersistenceElement.h"

PPMOutputHandle::PPMOutputHandle(java::io::OutputStream *_outputStream, int w, int h) {
    ImageOutputHandle::init("PPM", w, h);
    outputStream = _outputStream;

    if ( outputStream != nullptr ) {
        char header[64];
        const int headerLength = java::util::Formatter::format(
            header, static_cast<int>(sizeof(header)), "P6\n%d %d\n255\n", width, height);
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
    if ( outputStream != nullptr ) {
        vsdk::PersistenceElement::writeBytes(*outputStream, rgb, 3 * width);
        return width;
    }
    return 0;
}
