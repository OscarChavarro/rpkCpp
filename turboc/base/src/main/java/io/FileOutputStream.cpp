#include <stdio.h>

#include "java/io/File.h"
#include "java/io/FileOutputStream.h"

FILE *
FileOutputStream::toFileHandle(void *handle) {
    return ((FILE *)(handle));
}

FileOutputStream::FileOutputStream(const char *fileName):
    stream(NULL), ownsStream(TRUE)
{
    if ( fileName != NULL && fileName[0] != '\0' ) {
        const String platformName = File::toPlatformName(fileName);
        stream = ((void *)(fopen(platformName.toCString(), "wb")));
    }
}

FileOutputStream::FileOutputStream(FILE *openStream):
    stream((void *)(openStream)), ownsStream(FALSE)
{
}

FileOutputStream::~FileOutputStream() {
    close();
}

void
FileOutputStream::write(int value) {
    FILE *fileHandle = toFileHandle(stream);
    if ( fileHandle == NULL ) {
        return;
    }
    fputc(((unsigned char)(value & 0xFF)), fileHandle);
}

void
FileOutputStream::write(const unsigned char *buffer, int offset, int length) {
    FILE *fileHandle = toFileHandle(stream);
    if ( fileHandle == NULL || buffer == NULL || offset < 0 || length < 0 ) {
        return;
    }
    if ( length == 0 ) {
        return;
    }
    fwrite(buffer + offset, 1, ((size_t)(length)), fileHandle);
}

void
FileOutputStream::flush() {
    FILE *fileHandle = toFileHandle(stream);
    if ( fileHandle == NULL ) {
        return;
    }
    fflush(fileHandle);
}

void
FileOutputStream::close() {
    FILE *fileHandle = toFileHandle(stream);
    if ( fileHandle == NULL ) {
        return;
    }
    if ( ownsStream ) {
        fclose(fileHandle);
    }
    else {
        fflush(fileHandle);
    }
    stream = NULL;
}

void
FileOutputStream::dispose() {
    close();
}

