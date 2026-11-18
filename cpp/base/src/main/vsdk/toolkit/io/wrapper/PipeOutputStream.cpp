#include <cstdio>

#include "java/lang/ProcessBuilder.h"
#include "vsdk/toolkit/io/wrapper/PipeOutputStream.h"
FILE *
PipeOutputStream::toFileHandle(void *handle) {
    return static_cast<FILE *>(handle);
}

PipeOutputStream::PipeOutputStream(const char *command):
    pipeHandle(nullptr)
{
    if ( command != nullptr && command[0] != '\0' ) {
        java::ProcessBuilder processBuilder(command);
        pipeHandle = processBuilder.startWrite();
    }
}

PipeOutputStream::~PipeOutputStream() {
    close();
}

bool
PipeOutputStream::isOpen() const {
    return pipeHandle != nullptr;
}

void
PipeOutputStream::write(int value) {
    FILE *handle = toFileHandle(pipeHandle);
    if ( handle == nullptr ) {
        return;
    }
    fputc(static_cast<unsigned char>(value & 0xFF), handle);
}

void
PipeOutputStream::write(const unsigned char *buffer, int offset, int length) {
    FILE *handle = toFileHandle(pipeHandle);
    if ( handle == nullptr || buffer == nullptr || offset < 0 || length < 0 ) {
        return;
    }
    if ( length == 0 ) {
        return;
    }
    fwrite(buffer + offset, 1, static_cast<size_t>(length), handle);
}

void
PipeOutputStream::flush() {
    FILE *handle = toFileHandle(pipeHandle);
    if ( handle == nullptr ) {
        return;
    }
    fflush(handle);
}

void
PipeOutputStream::close() {
    FILE *handle = toFileHandle(pipeHandle);
    if ( handle == nullptr ) {
        return;
    }
    java::ProcessBuilder::close(handle);
    pipeHandle = nullptr;
}

void
PipeOutputStream::dispose() {
    close();
}
