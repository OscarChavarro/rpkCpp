#include "java/io/FileOutputStream.h"

#include "io/FileUncompressWrapper.h"

namespace java {
namespace io {

FileOutputStream::FileOutputStream():
    stream(nullptr),
    isPipe(false),
    standardOutput(false)
{
}

FileOutputStream::FileOutputStream(const File &file):
    stream(nullptr),
    isPipe(false),
    standardOutput(false)
{
    open(file);
}

FileOutputStream::~FileOutputStream() {
    close();
}

bool
FileOutputStream::open(const File &file) {
    return open(file.getPath().toCString());
}

bool
FileOutputStream::open(const char *fileName) {
    close();
    if ( fileName == nullptr || fileName[0] == '\0' ) {
        return false;
    }
    stream = fopen(fileName, "wb");
    isPipe = false;
    standardOutput = false;
    return stream != nullptr;
}

bool
FileOutputStream::open(FILE *fileHandle, bool pipeOutput) {
    close();
    if ( fileHandle == nullptr ) {
        return false;
    }
    stream = fileHandle;
    isPipe = pipeOutput ? 1 : 0;
    standardOutput = false;
    return true;
}

bool
FileOutputStream::openCompressed(const File &file) {
    return openCompressed(file.getPath().toCString());
}

bool
FileOutputStream::openCompressed(const char *fileName) {
    close();
    if ( fileName == nullptr || fileName[0] == '\0' ) {
        return false;
    }
    int pipeFlag = 0;
    stream = openFileCompressWrapper(fileName, "w", &pipeFlag);
    isPipe = pipeFlag;
    standardOutput = false;
    return stream != nullptr;
}

bool
FileOutputStream::openStandardOutput() {
    close();
    stream = stdout;
    isPipe = 0;
    standardOutput = true;
    return true;
}

bool
FileOutputStream::isOpen() const {
    return stream != nullptr;
}

bool
FileOutputStream::isPipeOutput() const {
    return isPipe != 0;
}

bool
FileOutputStream::isStandardOutput() const {
    return standardOutput;
}

int
FileOutputStream::write(int value) {
    if ( stream == nullptr ) {
        return -1;
    }
    unsigned char b[1];
    b[0] = static_cast<unsigned char>(value & 0xFF);
    const int written = write(b, 0, 1);
    return written == 1 ? (b[0] & 0xFF) : -1;
}

int
FileOutputStream::write(const unsigned char *buffer, int offset, int length) {
    if ( stream == nullptr || buffer == nullptr || offset < 0 || length < 0 ) {
        return -1;
    }
    const size_t written = fwrite(buffer + offset, 1, static_cast<size_t>(length), stream);
    return static_cast<int>(written);
}

FILE *
FileOutputStream::nativeHandle() const {
    return stream;
}

bool
FileOutputStream::close() {
    if ( stream == nullptr ) {
        return true;
    }

    if ( standardOutput ) {
        fflush(stream);
        stream = nullptr;
        isPipe = 0;
        standardOutput = false;
        return true;
    }

    closeFile(stream, isPipe);
    stream = nullptr;
    isPipe = 0;
    standardOutput = false;
    return true;
}

void
FileOutputStream::dispose() {
    close();
}

}
}
