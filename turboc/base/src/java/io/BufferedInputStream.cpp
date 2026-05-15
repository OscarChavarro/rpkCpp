#include "java/io/BufferedInputStream.h"


BufferedInputStream::BufferedInputStream(InputStream *inputStream):
    inputStream(inputStream)
{
}

BufferedInputStream::~BufferedInputStream() {
    dispose();
}

int
BufferedInputStream::read() {
    if ( inputStream == NULL ) {
        return -1;
    }
    return inputStream->read();
}

int
BufferedInputStream::read(unsigned char *buffer, int offset, int length) {
    if ( inputStream == NULL ) {
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
    if ( inputStream == NULL ) {
        return;
    }
    inputStream->dispose();
    inputStream = NULL;
}

