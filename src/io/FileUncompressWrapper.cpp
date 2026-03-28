#include <cstring>

#include "common/error.h"
#include "io/FileUncompressWrapper.h"
#include "java/io/File.h"
#include "java/io/FileInputStream.h"
#include "java/io/FileOutputStream.h"

namespace {

enum class StreamOpenMode {
    READ,
    WRITE
};

static const char *
modeToLogAction(StreamOpenMode mode) {
    return mode == StreamOpenMode::READ ? "reading" : "writing";
}

static const char *
modeToPopen(StreamOpenMode mode) {
    return mode == StreamOpenMode::READ ? "r" : "w";
}

static const char *
modeToFileOpen(StreamOpenMode mode) {
    return mode == StreamOpenMode::READ ? "rb" : "wb";
}

static FILE *
openFileHandleCompressWrapper(const char *fileName, StreamOpenMode openMode, int *isPipe) {
    if ( isPipe == nullptr ) {
        return nullptr;
    }
    *isPipe = 0;

    if ( fileName == nullptr || fileName[0] == '\0' || fileName[strlen(fileName) - 1] == '/' ) {
        return nullptr;
    }

    FILE *fileHandle = nullptr;
    const int commandLength = static_cast<int>(strlen(fileName)) + 20;
    char *command = new char[commandLength];
    const char *ext = strrchr(fileName, '.');

    if ( fileName[0] == '|' ) {
        std::snprintf(command, commandLength, "%s", &fileName[1]);
        fileHandle = popen(command, modeToPopen(openMode));
        *isPipe = 1;
    } else if ( ext && std::strcmp(ext, ".gz") == 0 ) {
        if ( openMode == StreamOpenMode::READ ) {
            std::snprintf(command, commandLength, "gunzip < %s", fileName);
        } else {
            std::snprintf(command, commandLength, "gzip > %s", fileName);
        }
        fileHandle = popen(command, modeToPopen(openMode));
        *isPipe = 1;
    } else if ( ext && std::strcmp(ext, ".Z") == 0 ) {
        if ( openMode == StreamOpenMode::READ ) {
            std::snprintf(command, commandLength, "uncompress < %s", fileName);
        } else {
            std::snprintf(command, commandLength, "compress > %s", fileName);
        }
        fileHandle = popen(command, modeToPopen(openMode));
        *isPipe = 1;
    } else if ( ext && std::strcmp(ext, ".bz") == 0 ) {
        if ( openMode == StreamOpenMode::READ ) {
            std::snprintf(command, commandLength, "bunzip < %s", fileName);
        } else {
            std::snprintf(command, commandLength, "bzip > %s", fileName);
        }
        fileHandle = popen(command, modeToPopen(openMode));
        *isPipe = 1;
    } else if ( ext && std::strcmp(ext, ".bz2") == 0 ) {
        if ( openMode == StreamOpenMode::READ ) {
            std::snprintf(command, commandLength, "bunzip2 < %s", fileName);
        } else {
            std::snprintf(command, commandLength, "bzip2 > %s", fileName);
        }
        fileHandle = popen(command, modeToPopen(openMode));
        *isPipe = 1;
    } else {
        fileHandle = java::io::File::openHandle(fileName, modeToFileOpen(openMode));
        *isPipe = 0;
    }

    delete[] command;

    if ( fileHandle == nullptr ) {
        logError(nullptr, "Can't open file '%s' for %s", fileName, modeToLogAction(openMode));
    }

    return fileHandle;
}

} // namespace

java::io::InputStream *
openInputStreamCompressWrapper(const char *fileName, int *isPipe) {
    int pipeFlag = 0;
    FILE *fileHandle = openFileHandleCompressWrapper(fileName, StreamOpenMode::READ, &pipeFlag);
    if ( fileHandle == nullptr ) {
        if ( isPipe != nullptr ) {
            *isPipe = 0;
        }
        return nullptr;
    }
    if ( isPipe != nullptr ) {
        *isPipe = pipeFlag;
    }

    return new java::io::FileInputStream(fileHandle, pipeFlag != 0, false);
}

java::io::OutputStream *
openOutputStreamCompressWrapper(const char *fileName, int *isPipe) {
    int pipeFlag = 0;
    FILE *fileHandle = openFileHandleCompressWrapper(fileName, StreamOpenMode::WRITE, &pipeFlag);
    if ( fileHandle == nullptr ) {
        if ( isPipe != nullptr ) {
            *isPipe = 0;
        }
        return nullptr;
    }
    if ( isPipe != nullptr ) {
        *isPipe = pipeFlag;
    }

    return new java::io::FileOutputStream(fileHandle, pipeFlag != 0, false);
}

void
closeInputStream(java::io::InputStream *stream) {
    if ( stream == nullptr ) {
        return;
    }
    stream->close();
    delete stream;
}

void
closeOutputStream(java::io::OutputStream *stream) {
    if ( stream == nullptr ) {
        return;
    }
    stream->close();
    delete stream;
}
