#include "java/io/File.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

namespace java {
namespace io {

namespace {
static bool
isValidPath(const char *rawPath) {
    return rawPath != nullptr && rawPath[0] != '\0';
}

static bool
canOpenForRead(const char *rawPath) {
    FILE *probe = std::fopen(rawPath, "rb");
    if ( probe == nullptr ) {
        return false;
    }
    std::fclose(probe);
    return true;
}

static bool
canOpenForUpdate(const char *rawPath) {
    FILE *probe = std::fopen(rawPath, "r+b");
    if ( probe == nullptr ) {
        return false;
    }
    std::fclose(probe);
    return true;
}

static bool
isDirectoryByReadProbe(const char *rawPath) {
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

static char *
buildDirectoryProbePath(const char *directoryPath) {
    if ( !isValidPath(directoryPath) ) {
        return nullptr;
    }

    const char *suffix = ".rpk_file_can_write_probe.tmp";
    const std::size_t dirLength = std::strlen(directoryPath);
    const std::size_t suffixLength = std::strlen(suffix);
    const bool hasSeparator = (dirLength > 0)
                              && (directoryPath[dirLength - 1] == '/' || directoryPath[dirLength - 1] == '\\');
    const std::size_t separatorLength = hasSeparator ? 0 : 1;
    const std::size_t totalLength = dirLength + separatorLength + suffixLength + 1;

    char *probePath = new char[totalLength];
    std::strcpy(probePath, directoryPath);
    if ( !hasSeparator ) {
        std::strcat(probePath, "/");
    }
    std::strcat(probePath, suffix);
    return probePath;
}

static bool
canCreateProbeFileInDirectory(const char *directoryPath) {
    char *probePath = buildDirectoryProbePath(directoryPath);
    if ( probePath == nullptr ) {
        return false;
    }

    const bool probeAlreadyExists = canOpenForRead(probePath) || canOpenForUpdate(probePath);
    if ( probeAlreadyExists ) {
        const bool writable = canOpenForUpdate(probePath);
        delete[] probePath;
        return writable;
    }

    FILE *probe = std::fopen(probePath, "wb");
    if ( probe == nullptr ) {
        delete[] probePath;
        return false;
    }

    std::fclose(probe);
    std::remove(probePath);
    delete[] probePath;
    return true;
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
    const char *rawPath = path.toCString();
    if ( !isValidPath(rawPath) ) {
        return false;
    }

    if ( canOpenForRead(rawPath) || canOpenForUpdate(rawPath) || isDirectoryByReadProbe(rawPath) ) {
        return true;
    }
    return canCreateProbeFileInDirectory(rawPath);
}

bool
File::isDirectory() const {
    const char *rawPath = path.toCString();
    if ( !isValidPath(rawPath) ) {
        return false;
    }

    if ( isDirectoryByReadProbe(rawPath) ) {
        return true;
    }
    if ( canOpenForRead(rawPath) || canOpenForUpdate(rawPath) ) {
        return false;
    }
    return canCreateProbeFileInDirectory(rawPath);
}

bool
File::canRead() const {
    const char *rawPath = path.toCString();
    if ( !isValidPath(rawPath) ) {
        return false;
    }
    return canOpenForRead(rawPath);
}

bool
File::canWrite() const {
    const char *rawPath = path.toCString();
    if ( !isValidPath(rawPath) ) {
        return false;
    }

    if ( canOpenForUpdate(rawPath) ) {
        return true;
    }
    return canCreateProbeFileInDirectory(rawPath);
}

}
}
