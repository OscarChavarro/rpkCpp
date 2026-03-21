#include "java/io/FileInputStream.h"

#include <cstring>

#include "io/FileUncompressWrapper.h"

namespace java {
namespace io {

FileInputStream::FileInputStream():
    stream(nullptr),
    isPipe(false),
    standardInput(false)
{
}

FileInputStream::FileInputStream(const File &file):
    stream(nullptr),
    isPipe(false),
    standardInput(false)
{
    open(file);
}

FileInputStream::~FileInputStream() {
    close();
}

bool
FileInputStream::open(const File &file) {
    return open(file.getPath().toCString());
}

bool
FileInputStream::open(const char *fileName) {
    close();
    int pipeFlag = false;
    stream = openFileCompressWrapper(fileName, "r", &pipeFlag);
    isPipe = pipeFlag;
    standardInput = false;
    return stream != nullptr;
}

bool
FileInputStream::openStandardInput() {
    close();
    stream = stdin;
    isPipe = false;
    standardInput = true;
    return true;
}

bool
FileInputStream::isOpen() const {
    return stream != nullptr;
}

bool
FileInputStream::isPipeInput() const {
    return isPipe;
}

bool
FileInputStream::isStandardInput() const {
    return standardInput;
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
    if ( stream == nullptr || buffer == nullptr || offset < 0 || length <= 0 ) {
        return 0;
    }
    return static_cast<int>(fread(buffer + offset, 1, static_cast<std::size_t>(length), stream));
}

int
FileInputStream::readLine(char *buffer, int maxLength) {
    if ( stream == nullptr || buffer == nullptr || maxLength <= 0 ) {
        return 0;
    }
    if ( fgets(buffer, maxLength, stream) == nullptr ) {
        return 0;
    }
    return static_cast<int>(std::strlen(buffer));
}

long
FileInputStream::tell() const {
    if ( stream == nullptr ) {
        return -1;
    }
    return ftell(stream);
}

bool
FileInputStream::seek(long offset) {
    if ( stream == nullptr ) {
        return false;
    }
    return fseek(stream, offset, 0) != EOF;
}

FILE *
FileInputStream::nativeHandle() const {
    return stream;
}

bool
FileInputStream::close() {
    if ( stream == nullptr ) {
        return true;
    }
    if ( !standardInput ) {
        closeFile(stream, isPipe);
    }
    stream = nullptr;
    isPipe = false;
    standardInput = false;
    return true;
}

}
}
