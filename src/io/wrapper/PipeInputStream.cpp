#include <cstdio>

#include "io/wrapper/PipeInputStream.h"

namespace {
static FILE *
toFileHandle(void *handle) {
    return static_cast<FILE *>(handle);
}
}

PipeInputStream::PipeInputStream(const char *command):
    pipeHandle(nullptr)
{
    if ( command != nullptr && command[0] != '\0' ) {
        pipeHandle = static_cast<void *>(popen(command, "r"));
    }
}

PipeInputStream::~PipeInputStream() {
    close();
}

bool
PipeInputStream::isOpen() const {
    return pipeHandle != nullptr;
}

int
PipeInputStream::read() {
    FILE *handle = toFileHandle(pipeHandle);
    if ( handle == nullptr ) {
        return -1;
    }
    return fgetc(handle);
}

int
PipeInputStream::read(unsigned char *buffer, int offset, int length) {
    FILE *handle = toFileHandle(pipeHandle);
    if ( handle == nullptr ) {
        return -1;
    }
    if ( buffer == nullptr || offset < 0 || length < 0 ) {
        return -1;
    }
    if ( length == 0 ) {
        return 0;
    }
    const int readCount = static_cast<int>(fread(&buffer[offset], 1, static_cast<size_t>(length), handle));
    if ( readCount == 0 && feof(handle) ) {
        return -1;
    }
    return readCount;
}

void
PipeInputStream::close() {
    FILE *handle = toFileHandle(pipeHandle);
    if ( handle == nullptr ) {
        return;
    }
    pclose(handle);
    pipeHandle = nullptr;
}

void
PipeInputStream::dispose() {
    close();
}
