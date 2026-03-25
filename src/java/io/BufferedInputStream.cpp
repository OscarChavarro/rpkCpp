#include "java/io/BufferedInputStream.h"

namespace java {
namespace io {

BufferedInputStream::BufferedInputStream(FileInputStream *inputStream, bool ownInputStream):
    inputStream(inputStream),
    ownInputStream(ownInputStream)
{
}

BufferedInputStream::~BufferedInputStream() {
    dispose();
}

bool
BufferedInputStream::open(const File &file) {
    if ( inputStream == nullptr ) {
        inputStream = new FileInputStream();
        ownInputStream = true;
    }
    return inputStream->open(file);
}

bool
BufferedInputStream::open(const char *fileName) {
    if ( inputStream == nullptr ) {
        inputStream = new FileInputStream();
        ownInputStream = true;
    }
    return inputStream->open(fileName);
}

bool
BufferedInputStream::openCompressed(const File &file) {
    if ( inputStream == nullptr ) {
        inputStream = new FileInputStream();
        ownInputStream = true;
    }
    return inputStream->openCompressed(file);
}

bool
BufferedInputStream::openCompressed(const char *fileName) {
    if ( inputStream == nullptr ) {
        inputStream = new FileInputStream();
        ownInputStream = true;
    }
    return inputStream->openCompressed(fileName);
}

bool
BufferedInputStream::openStandardInput() {
    if ( inputStream == nullptr ) {
        inputStream = new FileInputStream();
        ownInputStream = true;
    }
    return inputStream->openStandardInput();
}

bool
BufferedInputStream::isOpen() const {
    return inputStream != nullptr && inputStream->isOpen();
}

bool
BufferedInputStream::isPipeInput() const {
    return inputStream != nullptr && inputStream->isPipeInput();
}

bool
BufferedInputStream::isStandardInput() const {
    return inputStream != nullptr && inputStream->isStandardInput();
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
        return 0;
    }
    return inputStream->read(buffer, offset, length);
}

int
BufferedInputStream::readLine(char *buffer, int maxLength) {
    if ( inputStream == nullptr ) {
        return 0;
    }
    return inputStream->readLine(buffer, maxLength);
}

long
BufferedInputStream::tell() const {
    if ( inputStream == nullptr ) {
        return -1;
    }
    return inputStream->tell();
}

bool
BufferedInputStream::seek(long offset) {
    if ( inputStream == nullptr ) {
        return false;
    }
    return inputStream->seek(offset);
}

FILE *
BufferedInputStream::nativeHandle() const {
    if ( inputStream == nullptr ) {
        return nullptr;
    }
    return inputStream->nativeHandle();
}

bool
BufferedInputStream::close() {
    dispose();
    return true;
}

void
BufferedInputStream::dispose() {
    if ( inputStream == nullptr ) {
        return;
    }
    inputStream->dispose();
    if ( ownInputStream ) {
        delete inputStream;
    }
    inputStream = nullptr;
}

}
}
