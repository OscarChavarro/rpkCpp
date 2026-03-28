#include "java/io/FileOutputStream.h"

namespace java {
namespace io {

FileOutputStream::FileOutputStream():
    stream(nullptr),
    closeOnDispose(true)
{
}

FileOutputStream::FileOutputStream(const File &file):
    stream(file.open("wb")),
    closeOnDispose(true)
{
}

FileOutputStream::FileOutputStream(const char *fileName):
    stream(nullptr),
    closeOnDispose(true)
{
    stream = File::openHandle(fileName, "wb");
}

FileOutputStream::FileOutputStream(FILE *fileHandle, bool closeHandleOnDispose):
    stream(fileHandle),
    closeOnDispose(closeHandleOnDispose)
{
}

FileOutputStream::~FileOutputStream() {
    close();
}

bool
FileOutputStream::isOpen() const {
    return stream != nullptr;
}

bool
FileOutputStream::ownsHandle() const {
    return closeOnDispose;
}

void
FileOutputStream::write(int value) {
    if ( stream == nullptr ) {
        return;
    }
    fputc(static_cast<unsigned char>(value & 0xFF), stream);
}

void
FileOutputStream::write(const unsigned char *buffer, int offset, int length) {
    if ( stream == nullptr || buffer == nullptr || offset < 0 || length < 0 ) {
        return;
    }
    if ( length == 0 ) {
        return;
    }
    fwrite(buffer + offset, 1, static_cast<size_t>(length), stream);
}

void
FileOutputStream::close() {
    if ( stream == nullptr ) {
        return;
    }

    if ( !closeOnDispose ) {
        fflush(stream);
    } else {
        File::closeHandle(stream);
    }
    stream = nullptr;
    closeOnDispose = true;
}

void
FileOutputStream::dispose() {
    close();
}

}
}
