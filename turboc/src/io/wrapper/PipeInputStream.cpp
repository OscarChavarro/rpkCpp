#include <stdio.h>

#include "java/lang/ProcessBuilder.h"
#include "io/wrapper/PipeInputStream.h"

FILE *
PipeInputStream::toFileHandle(void *handle) {
    return ((FILE *)(handle));
}

PipeInputStream::PipeInputStream(const char *command):
    pipeHandle(NULL)
{
    if ( command != NULL && command[0] != '\0' ) {
        ProcessBuilder processBuilder(command);
        pipeHandle = processBuilder.startRead();
    }
}

PipeInputStream::~PipeInputStream() {
    close();
}

bool
PipeInputStream::isOpen() const {
    return pipeHandle != NULL;
}

int
PipeInputStream::read() {
    FILE *handle = toFileHandle(pipeHandle);
    if ( handle == NULL ) {
        return -1;
    }
    return fgetc(handle);
}

int
PipeInputStream::read(unsigned char *buffer, int offset, int length) {
    FILE *handle = toFileHandle(pipeHandle);
    if ( handle == NULL ) {
        return -1;
    }
    if ( buffer == NULL || offset < 0 || length < 0 ) {
        return -1;
    }
    if ( length == 0 ) {
        return 0;
    }
    const int readCount = ((int)(fread(&buffer[offset], 1, ((size_t)(length)), handle)));
    if ( readCount == 0 && feof(handle) ) {
        return -1;
    }
    return readCount;
}

void
PipeInputStream::close() {
    FILE *handle = toFileHandle(pipeHandle);
    if ( handle == NULL ) {
        return;
    }
    ProcessBuilder::close(handle);
    pipeHandle = NULL;
}

void
PipeInputStream::dispose() {
    close();
}
