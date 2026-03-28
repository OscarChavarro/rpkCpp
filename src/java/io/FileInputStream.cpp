#include "java/io/FileInputStream.h"

namespace java {
namespace io {

FileInputStream::FileInputStream():
    stream(nullptr),
    closeOnDispose(true)
{
}

FileInputStream::FileInputStream(const File &file):
    stream(file.open("rb")),
    closeOnDispose(true)
{
}

FileInputStream::FileInputStream(const char *fileName):
    stream(nullptr),
    closeOnDispose(true)
{
    stream = File::openHandle(fileName, "rb");
}

FileInputStream::FileInputStream(FILE *fileHandle, bool closeHandleOnDispose):
    stream(fileHandle),
    closeOnDispose(closeHandleOnDispose)
{
}

FileInputStream::~FileInputStream() {
    dispose();
}

bool
FileInputStream::isOpen() const {
    return stream != nullptr;
}

bool
FileInputStream::ownsHandle() const {
    return closeOnDispose;
}

int
FileInputStream::read() {
    if ( stream == nullptr ) {
        return -1;
    }
    return fgetc(stream);
}

int
FileInputStream::read(unsigned char *buffer, int offset, int length) {
    if ( stream == nullptr ) {
        return -1;
    }
    if ( buffer == nullptr || offset < 0 || length < 0 ) {
        return -1;
    }
    if ( length == 0 ) {
        return 0;
    }
    const int readCount = static_cast<int>(fread(&buffer[offset], 1, static_cast<std::size_t>(length), stream));
    if ( readCount == 0 && feof(stream) ) {
        return -1;
    }
    return readCount;
}

long
FileInputStream::tell() const {
    if ( stream == nullptr ) {
        return -1;
    }
    return ftell(stream);
}

void
FileInputStream::close() {
    if ( stream == nullptr ) {
        return;
    }
    if ( closeOnDispose ) {
        File::closeHandle(stream);
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
