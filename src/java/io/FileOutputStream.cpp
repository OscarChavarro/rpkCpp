#include "java/io/FileOutputStream.h"

namespace java {
namespace io {

FileOutputStream::FileOutputStream():
    stream(nullptr),
    isPipe(false),
    standardOutput(false)
{
}

FileOutputStream::FileOutputStream(const File &file):
    stream(file.open("wb")),
    isPipe(false),
    standardOutput(false)
{
}

FileOutputStream::FileOutputStream(const char *fileName):
    stream(nullptr),
    isPipe(false),
    standardOutput(false)
{
    stream = File::openHandle(fileName, "wb");
}

FileOutputStream::FileOutputStream(FILE *fileHandle, bool pipeOutput, bool isStandardOutput):
    stream(fileHandle),
    isPipe(pipeOutput ? 1 : 0),
    standardOutput(isStandardOutput)
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
FileOutputStream::isPipeOutput() const {
    return isPipe != 0;
}

bool
FileOutputStream::isStandardOutput() const {
    return standardOutput;
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

    if ( standardOutput ) {
        fflush(stream);
        stream = nullptr;
        isPipe = 0;
        standardOutput = false;
        return;
    }

    if ( isPipe ) {
        pclose(stream);
    } else {
        File::closeHandle(stream);
    }
    stream = nullptr;
    isPipe = 0;
    standardOutput = false;
}

void
FileOutputStream::dispose() {
    close();
}

}
}
