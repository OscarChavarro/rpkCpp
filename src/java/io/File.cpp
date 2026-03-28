#include "java/io/File.h"

#include <sys/stat.h>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace java {
namespace io {

namespace {
static bool
isValidPath(const char *rawPath) {
    return rawPath != nullptr && rawPath[0] != '\0';
}

static bool
queryStat(const char *rawPath, struct stat *fileInfo) {
    if ( !isValidPath(rawPath) || fileInfo == nullptr ) {
        return false;
    }
    return ::stat(rawPath, fileInfo) == 0;
}

static int
pathAccess(const char *rawPath, int mode) {
#if defined(_WIN32)
    return _access(rawPath, mode);
#else
    return access(rawPath, mode);
#endif
}
}

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

bool
File::isEmpty() const {
    return path.isEmpty();
}

bool
File::exists() const {
    struct stat fileInfo {};
    return queryStat(path.toCString(), &fileInfo);
}

bool
File::isDirectory() const {
    struct stat fileInfo {};
    if ( !queryStat(path.toCString(), &fileInfo) ) {
        return false;
    }
#if defined(_WIN32)
    return (fileInfo.st_mode & _S_IFDIR) != 0;
#else
    return S_ISDIR(fileInfo.st_mode);
#endif
}

bool
File::canRead() const {
    const char *rawPath = path.toCString();
    if ( !isValidPath(rawPath) ) {
        return false;
    }
#if defined(_WIN32)
    return pathAccess(rawPath, 4) == 0;
#else
    return pathAccess(rawPath, R_OK) == 0;
#endif
}

bool
File::canWrite() const {
    const char *rawPath = path.toCString();
    if ( !isValidPath(rawPath) ) {
        return false;
    }
#if defined(_WIN32)
    return pathAccess(rawPath, 2) == 0;
#else
    return pathAccess(rawPath, W_OK) == 0;
#endif
}

}
}
