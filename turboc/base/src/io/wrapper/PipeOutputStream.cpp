#include <stdio.h>

#include "java/lang/ProcessBuilder.h"
#include "io/wrapper/PipeOutputStream.h"

FILE *
PipeOutputStream::toFileHandle(void *handle) {
    return ((FILE *)(handle));
}

PipeOutputStream::PipeOutputStream(const char *command):
    pipeHandle(NULL)
{
    if ( command != NULL && command[0] != '\0' ) {
        ProcessBuilder processBuilder(command);
        pipeHandle = processBuilder.startWrite();
    }
}

PipeOutputStream::~PipeOutputStream() {
    close();
}

bool
PipeOutputStream::isOpen() const {
    return pipeHandle != NULL;
}

void
PipeOutputStream::write(int value) {
    FILE *handle = toFileHandle(pipeHandle);
    if ( handle == NULL ) {
        return;
    }
    fputc(((unsigned char)(value & 0xFF)), handle);
}

void
PipeOutputStream::write(const unsigned char *buffer, int offset, int length) {
    FILE *handle = toFileHandle(pipeHandle);
    if ( handle == NULL || buffer == NULL || offset < 0 || length < 0 ) {
        return;
    }
    if ( length == 0 ) {
        return;
    }
    fwrite(buffer + offset, 1, ((size_t)(length)), handle);
}

void
PipeOutputStream::flush() {
    FILE *handle = toFileHandle(pipeHandle);
    if ( handle == NULL ) {
        return;
    }
    fflush(handle);
}

void
PipeOutputStream::close() {
    FILE *handle = toFileHandle(pipeHandle);
    if ( handle == NULL ) {
        return;
    }
    ProcessBuilder::close(handle);
    pipeHandle = NULL;
}

void
PipeOutputStream::dispose() {
    close();
}
