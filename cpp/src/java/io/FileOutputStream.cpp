#include <cstdio>

#include "java/io/FileOutputStream.h"

namespace java {

FILE *
FileOutputStream::toFileHandle(void *handle) {
    return static_cast<FILE *>(handle);
}

FileOutputStream::FileOutputStream(const char *fileName):
    stream(nullptr)
{
    if ( fileName != nullptr && fileName[0] != '\0' ) {
        stream = static_cast<void *>(std::fopen(fileName, "wb"));
    }
}

FileOutputStream::~FileOutputStream() {
    close();
}

void
FileOutputStream::write(int value) {
    FILE *fileHandle = toFileHandle(stream);
    if ( fileHandle == nullptr ) {
        return;
    }
    fputc(static_cast<unsigned char>(value & 0xFF), fileHandle);
}

void
FileOutputStream::write(const unsigned char *buffer, int offset, int length) {
    FILE *fileHandle = toFileHandle(stream);
    if ( fileHandle == nullptr || buffer == nullptr || offset < 0 || length < 0 ) {
        return;
    }
    if ( length == 0 ) {
        return;
    }
    fwrite(buffer + offset, 1, static_cast<size_t>(length), fileHandle);
}

void
FileOutputStream::flush() {
    FILE *fileHandle = toFileHandle(stream);
    if ( fileHandle == nullptr ) {
        return;
    }
    fflush(fileHandle);
}

void
FileOutputStream::close() {
    FILE *fileHandle = toFileHandle(stream);
    if ( fileHandle == nullptr ) {
        return;
    }
    std::fclose(fileHandle);
    stream = nullptr;
}

void
FileOutputStream::dispose() {
    close();
}

}
