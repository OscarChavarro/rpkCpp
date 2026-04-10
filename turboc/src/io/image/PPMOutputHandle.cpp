#include "java/util/Formatter.h"
#include "io/wrapper/PersistenceElement.h"
#include "io/image/PPMOutputHandle.h"

PPMOutputHandle::PPMOutputHandle(OutputStream *_outputStream, int w, int h) {
    ImageOutputHandle::init("PPM", w, h);
    outputStream = _outputStream;

    if ( outputStream != NULL ) {
        char header[64];
        const int headerLength = Formatter::format(
            header, ((int)(sizeof(header))), "P6\n%d %d\n255\n", width, height);
        if ( headerLength > 0 ) {
            PersistenceElement::writeBytes(
                *outputStream,
                ((const unsigned char *)(header)),
                headerLength);
        }
    }
}

int
PPMOutputHandle::writeDisplayRGB(unsigned char *rgb) {
    if ( outputStream != NULL ) {
        PersistenceElement::writeBytes(*outputStream, rgb, 3 * width);
        return width;
    }
    return 0;
}
