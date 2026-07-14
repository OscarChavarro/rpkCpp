#include <cstring>

#include "java/io/FileInputStream.h"
#include "java/io/FileOutputStream.h"
#include "java/util/Formatter.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/io/wrapper/FileUncompressWrapper.h"
#include "vsdk/toolkit/io/wrapper/PipeInputStream.h"
#include "vsdk/toolkit/io/wrapper/PipeOutputStream.h"
#include "vsdk/toolkit/io/wrapper/StreamOpenMode.h"

const char *
FileUncompressWrapper::modeToLogAction(StreamOpenMode mode) {
    return mode == StreamOpenMode::READ ? "reading" : "writing";
}

bool
FileUncompressWrapper::isInvalidFileName(const char *fileName) {
    if ( fileName == nullptr || fileName[0] == '\0' || fileName[strlen(fileName) - 1] == '/' ) {
        return true;
    }
    return false;
}

bool
FileUncompressWrapper::buildPipeCommand(const char *fileName, StreamOpenMode openMode, char *command, int commandLength) {
    if ( fileName == nullptr || command == nullptr || commandLength <= 0 ) {
        return false;
    }

    if ( fileName[0] == '|' ) {
        java::Formatter::format(command, commandLength, "%s", &fileName[1]);
        return true;
    }

    const char *ext = strrchr(fileName, '.');
    if ( ext && std::strcmp(ext, ".gz") == 0 ) {
        if ( openMode == StreamOpenMode::READ ) {
            java::Formatter::format(command, commandLength, "gunzip < %s", fileName);
        } else {
            java::Formatter::format(command, commandLength, "gzip > %s", fileName);
        }
    } else if ( ext && std::strcmp(ext, ".Z") == 0 ) {
        if ( openMode == StreamOpenMode::READ ) {
            java::Formatter::format(command, commandLength, "uncompress < %s", fileName);
        } else {
            java::Formatter::format(command, commandLength, "compress > %s", fileName);
        }
    } else if ( ext && std::strcmp(ext, ".bz") == 0 ) {
        if ( openMode == StreamOpenMode::READ ) {
            java::Formatter::format(command, commandLength, "bunzip < %s", fileName);
        } else {
            java::Formatter::format(command, commandLength, "bzip > %s", fileName);
        }
    } else if ( ext && std::strcmp(ext, ".bz2") == 0 ) {
        if ( openMode == StreamOpenMode::READ ) {
            java::Formatter::format(command, commandLength, "bunzip2 < %s", fileName);
        } else {
            java::Formatter::format(command, commandLength, "bzip2 > %s", fileName);
        }
    } else {
        return false;
    }
    return true;
}

java::InputStream *
FileUncompressWrapper::openPipeInputStream(const char *command) {
    PipeInputStream *pipeStream = new PipeInputStream(command);
    if ( !pipeStream->isOpen() ) {
        delete pipeStream;
        return nullptr;
    }
    return pipeStream;
}

java::OutputStream *
FileUncompressWrapper::openPipeOutputStream(const char *command) {
    PipeOutputStream *pipeStream = new PipeOutputStream(command);
    if ( !pipeStream->isOpen() ) {
        delete pipeStream;
        return nullptr;
    }
    return pipeStream;
}

java::InputStream *
FileUncompressWrapper::openInputStreamCompressWrapper(const char *fileName, int *isPipe) {
    if ( isPipe != nullptr ) {
        *isPipe = 0;
    }
    if ( isInvalidFileName(fileName) ) {
        return nullptr;
    }

    const int commandLength = static_cast<int>(strlen(fileName)) + 20;
    char *command = new char[commandLength];
    const bool pipeFlag = buildPipeCommand(fileName, StreamOpenMode::READ, command, commandLength);

    java::InputStream *stream = nullptr;
    if ( pipeFlag ) {
        stream = openPipeInputStream(command);
    } else {
        java::File file(fileName);
        if ( file.exists() && file.canRead() && file.isFile() ) {
            stream = new java::FileInputStream(fileName);
        }
    }
    delete[] command;

    if ( stream == nullptr ) {
        Logger::error(nullptr, "Can't open file '%s' for %s", fileName, modeToLogAction(StreamOpenMode::READ));
        if ( isPipe != nullptr ) {
            *isPipe = 0;
        }
        return nullptr;
    }

    if ( isPipe != nullptr ) {
        *isPipe = pipeFlag ? 1 : 0;
    }
    return stream;
}

java::OutputStream *
FileUncompressWrapper::openOutputStreamCompressWrapper(const char *fileName, int *isPipe) {
    if ( isPipe != nullptr ) {
        *isPipe = 0;
    }
    if ( isInvalidFileName(fileName) ) {
        return nullptr;
    }

    const int commandLength = static_cast<int>(strlen(fileName)) + 20;
    char *command = new char[commandLength];
    const bool pipeFlag = buildPipeCommand(fileName, StreamOpenMode::WRITE, command, commandLength);

    java::OutputStream *stream = nullptr;
    if ( pipeFlag ) {
        stream = openPipeOutputStream(command);
    } else {
        java::File file(fileName);
        if ( file.canWrite() && !file.isDirectory() ) {
            stream = new java::FileOutputStream(fileName);
        }
    }
    delete[] command;

    if ( stream == nullptr ) {
        Logger::error(nullptr, "Can't open file '%s' for %s", fileName, modeToLogAction(StreamOpenMode::WRITE));
        if ( isPipe != nullptr ) {
            *isPipe = 0;
        }
        return nullptr;
    }

    if ( isPipe != nullptr ) {
        *isPipe = pipeFlag ? 1 : 0;
    }
    return stream;
}

void
FileUncompressWrapper::closeInputStream(java::InputStream *stream) {
    if ( stream == nullptr ) {
        return;
    }
    stream->close();
    delete stream;
}

void
FileUncompressWrapper::closeOutputStream(java::OutputStream *stream) {
    if ( stream == nullptr ) {
        return;
    }
    stream->close();
    delete stream;
}
