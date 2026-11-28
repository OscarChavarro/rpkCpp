#include "java/io/BufferedOutputStream.h"
namespace java {

BufferedOutputStream::BufferedOutputStream(OutputStream *outputStream):
    outputStream(outputStream)
{
}

BufferedOutputStream::~BufferedOutputStream() {
    dispose();
}

void
BufferedOutputStream::write(int value) {
    if (outputStream == nullptr) {
        return;
    }
    outputStream->write(value);
}

void
BufferedOutputStream::write(const unsigned char *buffer, int offset, int length) {
    if (outputStream == nullptr) {
        return;
    }
    outputStream->write(buffer, offset, length);
}

void
BufferedOutputStream::flush() {
    if (outputStream == nullptr) {
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
    if (outputStream == nullptr) {
        return;
    }
    outputStream->dispose();
    outputStream = nullptr;
}

}
