#include "java/io/FileInputStream.h"

#include <cstdio>

namespace java {
namespace io {

namespace {
static FILE *
toFileHandle(void *handle) {
    return static_cast<FILE *>(handle);
}
}

FileInputStream::FileInputStream():
    stream(nullptr),
    closeOnDispose(true)
{
}

FileInputStream::FileInputStream(const char *fileName):
    stream(nullptr),
    closeOnDispose(true)
{
    if ( fileName != nullptr && fileName[0] != '\0' ) {
        stream = static_cast<void *>(std::fopen(fileName, "rb"));
    }
}

FileInputStream::~FileInputStream() {
    dispose();
}

bool
FileInputStream::isOpen() const {
    return toFileHandle(stream) != nullptr;
}

int
FileInputStream::read() {
    FILE *fileHandle = toFileHandle(stream);
    if ( fileHandle == nullptr ) {
        return -1;
    }
    return fgetc(fileHandle);
}

int
FileInputStream::read(unsigned char *buffer, int offset, int length) {
    FILE *fileHandle = toFileHandle(stream);
    if ( fileHandle == nullptr ) {
        return -1;
    }
    if ( buffer == nullptr || offset < 0 || length < 0 ) {
        return -1;
    }
    if ( length == 0 ) {
        return 0;
    }
    const int readCount = static_cast<int>(fread(&buffer[offset], 1, static_cast<std::size_t>(length), fileHandle));
    if ( readCount == 0 && feof(fileHandle) ) {
        return -1;
    }
    return readCount;
}

long
FileInputStream::tell() const {
    FILE *fileHandle = toFileHandle(stream);
    if ( fileHandle == nullptr ) {
        return -1;
    }
    return ftell(fileHandle);
}

void
FileInputStream::close() {
    FILE *fileHandle = toFileHandle(stream);
    if ( fileHandle == nullptr ) {
        return;
    }
    if ( closeOnDispose ) {
        std::fclose(fileHandle);
    }
    stream = nullptr;
    closeOnDispose = true;
}

void
FileInputStream::dispose() {
    close();
}

}
}
