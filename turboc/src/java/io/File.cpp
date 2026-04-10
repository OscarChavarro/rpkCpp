#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "java/io/File.h"


bool
File::isValidPath(const char *rawPath) {
    return rawPath != NULL && rawPath[0] != '\0';
}

bool
File::canOpenWithMode(const char *rawPath, const char *mode, int *errorCode) {
    errno = 0;
    FILE *probe = fopen(rawPath, mode);
    if ( probe == NULL ) {
        if ( errorCode != NULL ) {
            *errorCode = errno;
        }
        return false;
    }
    fclose(probe);
    if ( errorCode != NULL ) {
        *errorCode = 0;
    }
    return true;
}

bool
File::isDirectoryByReadProbe(const char *rawPath) {
    int openError = 0;
    if ( canOpenWithMode(rawPath, "rb", &openError) ) {
        errno = 0;
        FILE *probe = fopen(rawPath, "rb");
        if ( probe == NULL ) {
            return false;
        }
        unsigned char byteProbe = 0;
        errno = 0;
        const size_t readCount = fread(&byteProbe, 1, 1, probe);
        const bool isDirectory = (readCount == 0) && (ferror(probe) != 0) && (errno == EISDIR);
        fclose(probe);
        return isDirectory;
    }

    if ( openError == EISDIR ) {
        return true;
    }

    const size_t pathLength = strlen(rawPath);
    char *slashPath = new char[pathLength + 2];
    memcpy(slashPath, rawPath, pathLength);
    slashPath[pathLength] = '/';
    slashPath[pathLength + 1] = '\0';

    int slashError = 0;
    const bool slashOpenOk = canOpenWithMode(slashPath, "rb", &slashError);
    delete[] slashPath;
    if ( slashOpenOk ) {
        return true;
    }
    return slashError == EISDIR || slashError == EACCES || slashError == EPERM;
}

File::File():
    path()
{
}

File::File(const char *path):
    path(path)
{
}

File::File(const String &path):
    path(path)
{
}

File::~File() {
    dispose();
}

void
File::dispose() {
    path.dispose();
}

String
File::getName() const {
    const int separator = path.indexOf('/');
    if ( separator < 0 ) {
        return path;
    }
    int tailStart = separator + 1;
    int lastSeparator = separator;
    while ( true ) {
        const int nextSeparator = path.indexOf('/', tailStart);
        if ( nextSeparator < 0 ) {
            break;
        }
        lastSeparator = nextSeparator;
        tailStart = nextSeparator + 1;
    }
    return path.substring(lastSeparator + 1);
}

bool
File::exists() const {
    const char *rawPath = path.toCString();
    if ( !isValidPath(rawPath) ) {
        return false;
    }

    int readError = 0;
    if ( canOpenWithMode(rawPath, "rb", &readError) ) {
        return true;
    }
    if ( readError == EACCES || readError == EPERM || readError == EISDIR ) {
        return true;
    }
    return isDirectoryByReadProbe(rawPath);
}

bool
File::isDirectory() const {
    const char *rawPath = path.toCString();
    if ( !isValidPath(rawPath) ) {
        return false;
    }

    return isDirectoryByReadProbe(rawPath);
}

bool
File::isFile() const {
    if ( !exists() ) {
        return false;
    }
    return !isDirectory();
}

bool
File::canRead() const {
    const char *rawPath = path.toCString();
    if ( !isValidPath(rawPath) ) {
        return false;
    }
    return canOpenWithMode(rawPath, "rb");
}

bool
File::canWrite() const {
    const char *rawPath = path.toCString();
    if ( !isValidPath(rawPath) ) {
        return false;
    }

    return canOpenWithMode(rawPath, "ab");
}

