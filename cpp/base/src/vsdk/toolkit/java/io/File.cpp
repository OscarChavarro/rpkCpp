#include <cerrno>
#include <cstdio>
#include <cstring>

#include "vsdk/toolkit/java/io/File.h"

namespace java {

bool
File::isValidPath(const char *rawPath) {
    return rawPath != nullptr && rawPath[0] != '\0';
}

bool
File::canOpenWithMode(const char *rawPath, const char *mode, int *errorCode) {
    errno = 0;
    FILE *probe = std::fopen(rawPath, mode);
    if ( probe == nullptr ) {
        if ( errorCode != nullptr ) {
            *errorCode = errno;
        }
        return false;
    }
    std::fclose(probe);
    if ( errorCode != nullptr ) {
        *errorCode = 0;
    }
    return true;
}

bool
File::isDirectoryByReadProbe(const char *rawPath) {
    int openError = 0;
    if ( canOpenWithMode(rawPath, "rb", &openError) ) {
        errno = 0;
        FILE *probe = std::fopen(rawPath, "rb");
        if ( probe == nullptr ) {
            return false;
        }
        unsigned char byteProbe = 0;
        errno = 0;
        const std::size_t readCount = std::fread(&byteProbe, 1, 1, probe);
        const bool isDirectory = (readCount == 0) && (std::ferror(probe) != 0) && (errno == EISDIR);
        std::fclose(probe);
        return isDirectory;
    }

    if ( openError == EISDIR ) {
        return true;
    }

    const std::size_t pathLength = std::strlen(rawPath);
    char *slashPath = new char[pathLength + 2];
    std::memcpy(slashPath, rawPath, pathLength);
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

File::File(const java::String &path):
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

java::String
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

}
