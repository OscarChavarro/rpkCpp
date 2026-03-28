#include "java/io/File.h"

#include <cstdio>

namespace java {
namespace io {

File::File():
    path()
{
}

File::File(const char *path):
    path(path)
{
}

File::File(const java::lang::String &path):
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

const java::lang::String &
File::getPath() const {
    return path;
}

java::lang::String
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

java::lang::String
File::getParent() const {
    int lastSeparator = -1;
    int searchFrom = 0;
    while ( true ) {
        const int nextSeparator = path.indexOf('/', searchFrom);
        if ( nextSeparator < 0 ) {
            break;
        }
        lastSeparator = nextSeparator;
        searchFrom = nextSeparator + 1;
    }
    if ( lastSeparator < 0 ) {
        return java::lang::String();
    }
    return path.substring(0, lastSeparator);
}

FILE *
File::open(const char *openMode) const {
    return openHandle(path.toCString(), openMode);
}

FILE *
File::openHandle(const char *filePath, const char *openMode) {
    if ( filePath == nullptr || openMode == nullptr || filePath[0] == '\0' || openMode[0] == '\0' ) {
        return nullptr;
    }
    return std::fopen(filePath, openMode);
}

int
File::closeHandle(FILE *handle) {
    if ( handle == nullptr ) {
        return 0;
    }
    return std::fclose(handle);
}

bool
File::isEmpty() const {
    return path.isEmpty();
}

}
}
