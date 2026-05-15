#include <stdio.h>

#include "java/io/FileInputStream.h"


FILE *
FileInputStream::toFileHandle(void *handle) {
    return ((FILE *)(handle));
}

FileInputStream::FileInputStream(const char *fileName):
    stream(NULL)
{
    if ( fileName != NULL && fileName[0] != '\0' ) {
        stream = ((void *)(fopen(fileName, "rb")));
    }
}

FileInputStream::~FileInputStream() {
    dispose();
}

int
FileInputStream::read() {
    FILE *fileHandle = toFileHandle(stream);
    if ( fileHandle == NULL ) {
        return -1;
    }
    return fgetc(fileHandle);
}

int
FileInputStream::read(unsigned char *buffer, int offset, int length) {
    FILE *fileHandle = toFileHandle(stream);
    if ( fileHandle == NULL ) {
        return -1;
    }
    if ( buffer == NULL || offset < 0 || length < 0 ) {
        return -1;
    }
    if ( length == 0 ) {
        return 0;
    }
    const int readCount = ((int)(fread(&buffer[offset], 1, ((size_t)(length)), fileHandle)));
    if ( readCount == 0 && feof(fileHandle) ) {
        return -1;
    }
    return readCount;
}

void
FileInputStream::close() {
    FILE *fileHandle = toFileHandle(stream);
    if ( fileHandle == NULL ) {
        return;
    }
    fclose(fileHandle);
    stream = NULL;
}

void
FileInputStream::dispose() {
    close();
}

