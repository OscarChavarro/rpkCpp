#include <cstdio>
#include "java/util/Formatter.h"

#include <cstring>

#include "common/error.h"
#include "io/wrapper/FileUncompressWrapper.h"
#include "io/wrapper/PipeInputStream.h"
#include "io/wrapper/PipeOutputStream.h"
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

static bool
isInvalidFileName(const char *fileName) {
    if ( fileName == nullptr || fileName[0] == '\0' || fileName[strlen(fileName) - 1] == '/' ) {
        return true;
    }
    return false;
}

static int
buildPipeCommand(const char *fileName, StreamOpenMode openMode, char *command, int commandLength) {
    if ( fileName == nullptr || command == nullptr || commandLength <= 0 ) {
        return 0;
    }

    if ( fileName[0] == '|' ) {
        java::util::Formatter::format(command, commandLength, "%s", &fileName[1]);
        return 1;
    }

    const char *ext = strrchr(fileName, '.');
    if ( ext && std::strcmp(ext, ".gz") == 0 ) {
        if ( openMode == StreamOpenMode::READ ) {
            java::util::Formatter::format(command, commandLength, "gunzip < %s", fileName);
        } else {
            java::util::Formatter::format(command, commandLength, "gzip > %s", fileName);
        }
    } else if ( ext && std::strcmp(ext, ".Z") == 0 ) {
        if ( openMode == StreamOpenMode::READ ) {
            java::util::Formatter::format(command, commandLength, "uncompress < %s", fileName);
        } else {
            java::util::Formatter::format(command, commandLength, "compress > %s", fileName);
        }
    } else if ( ext && std::strcmp(ext, ".bz") == 0 ) {
        if ( openMode == StreamOpenMode::READ ) {
            java::util::Formatter::format(command, commandLength, "bunzip < %s", fileName);
        } else {
            java::util::Formatter::format(command, commandLength, "bzip > %s", fileName);
        }
    } else if ( ext && std::strcmp(ext, ".bz2") == 0 ) {
        if ( openMode == StreamOpenMode::READ ) {
            java::util::Formatter::format(command, commandLength, "bunzip2 < %s", fileName);
        } else {
            java::util::Formatter::format(command, commandLength, "bzip2 > %s", fileName);
        }
    } else {
        return 0;
    }
    return 1;
}

static java::io::InputStream *
openPipeInputStream(const char *command) {
    PipeInputStream *pipeStream = new PipeInputStream(command);
    if ( !pipeStream->isOpen() ) {
        delete pipeStream;
        return nullptr;
    }
    return pipeStream;
}

static java::io::OutputStream *
openPipeOutputStream(const char *command) {
    PipeOutputStream *pipeStream = new PipeOutputStream(command);
    if ( !pipeStream->isOpen() ) {
        delete pipeStream;
        return nullptr;
    }
    return pipeStream;
}

} // namespace

java::io::InputStream *
openInputStreamCompressWrapper(const char *fileName, int *isPipe) {
    if ( isPipe != nullptr ) {
        *isPipe = 0;
    }
    if ( isInvalidFileName(fileName) ) {
        return nullptr;
    }

    const int commandLength = static_cast<int>(strlen(fileName)) + 20;
    char *command = new char[commandLength];
    const int pipeFlag = buildPipeCommand(fileName, StreamOpenMode::READ, command, commandLength);

    java::io::InputStream *stream = nullptr;
    if ( pipeFlag != 0 ) {
        stream = openPipeInputStream(command);
    } else {
        java::io::File file(fileName);
        if ( file.exists() && file.canRead() && file.isFile() ) {
            stream = new java::io::FileInputStream(fileName);
        }
    }
    delete[] command;

    if ( stream == nullptr ) {
        logError(nullptr, "Can't open file '%s' for %s", fileName, modeToLogAction(StreamOpenMode::READ));
        if ( isPipe != nullptr ) {
            *isPipe = 0;
        }
        return nullptr;
    }

    if ( isPipe != nullptr ) {
        *isPipe = pipeFlag;
    }
    return stream;
}

java::io::OutputStream *
openOutputStreamCompressWrapper(const char *fileName, int *isPipe) {
    if ( isPipe != nullptr ) {
        *isPipe = 0;
    }
    if ( isInvalidFileName(fileName) ) {
        return nullptr;
    }

    const int commandLength = static_cast<int>(strlen(fileName)) + 20;
    char *command = new char[commandLength];
    const int pipeFlag = buildPipeCommand(fileName, StreamOpenMode::WRITE, command, commandLength);

    java::io::OutputStream *stream = nullptr;
    if ( pipeFlag != 0 ) {
        stream = openPipeOutputStream(command);
    } else {
        java::io::File file(fileName);
        if ( file.canWrite() && !file.isDirectory() ) {
            stream = new java::io::FileOutputStream(fileName);
        }
    }
    delete[] command;

    if ( stream == nullptr ) {
        logError(nullptr, "Can't open file '%s' for %s", fileName, modeToLogAction(StreamOpenMode::WRITE));
        if ( isPipe != nullptr ) {
            *isPipe = 0;
        }
        return nullptr;
    }

    if ( isPipe != nullptr ) {
        *isPipe = pipeFlag;
    }
    return stream;
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
