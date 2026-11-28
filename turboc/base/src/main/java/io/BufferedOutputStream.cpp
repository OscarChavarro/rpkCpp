#include <stddef.h>
#include "java/io/BufferedOutputStream.h"

BufferedOutputStream::BufferedOutputStream(OutputStream *outputStream):
    outputStream(outputStream)
{
}

BufferedOutputStream::~BufferedOutputStream() {
    dispose();
}

void
BufferedOutputStream::write(int value) {
    if (outputStream == NULL) {
        return;
    }
    outputStream->write(value);
}

void
BufferedOutputStream::write(const unsigned char *buffer, int offset, int length) {
    if (outputStream == NULL) {
        return;
    }
    outputStream->write(buffer, offset, length);
}

void
BufferedOutputStream::flush() {
    if (outputStream == NULL) {
        return;
    }
    outputStream->flush();
}

void
BufferedOutputStream::close() {
    dispose();
}

void
BufferedOutputStream::dispose() {
    if (outputStream == NULL) {
        return;
    }
    outputStream->dispose();
    outputStream = NULL;
}

