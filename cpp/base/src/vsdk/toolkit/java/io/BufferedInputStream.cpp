#include "vsdk/toolkit/java/io/BufferedInputStream.h"

namespace java {

BufferedInputStream::BufferedInputStream(InputStream *inputStream):
    inputStream(inputStream)
{
}

BufferedInputStream::~BufferedInputStream() {
    dispose();
}

int
BufferedInputStream::read() {
    if ( inputStream == nullptr ) {
        return -1;
    }
    return inputStream->read();
}

int
BufferedInputStream::read(unsigned char *buffer, int offset, int length) {
    if ( inputStream == nullptr ) {
        return -1;
    }
    return inputStream->read(buffer, offset, length);
}

void
BufferedInputStream::close() {
    dispose();
}

void
BufferedInputStream::dispose() {
    if ( inputStream == nullptr ) {
        return;
    }
    inputStream->dispose();
    inputStream = nullptr;
}

}
