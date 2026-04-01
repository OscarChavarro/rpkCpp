#include <cstring>

#include "java/io/FileInputStream.h"
#include "java/io/FileOutputStream.h"
#include "java/util/Formatter.h"
#include "common/Error.h"
#include "io/wrapper/FileUncompressWrapper.h"
#include "io/wrapper/PipeInputStream.h"
#include "io/wrapper/PipeOutputStream.h"
#include "io/wrapper/StreamOpenMode.h"

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

int
FileUncompressWrapper::buildPipeCommand(const char *fileName, StreamOpenMode openMode, char *command, int commandLength) {
    if ( fileName == nullptr || command == nullptr || commandLength <= 0 ) {
        return 0;
    }

    if ( fileName[0] == '|' ) {
        java::Formatter::format(command, commandLength, "%s", &fileName[1]);
        return 1;
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
        return 0;
    }
    return 1;
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
    const int pipeFlag = buildPipeCommand(fileName, StreamOpenMode::READ, command, commandLength);

    java::InputStream *stream = nullptr;
    if ( pipeFlag != 0 ) {
        stream = openPipeInputStream(command);
    } else {
        java::File file(fileName);
        if ( file.exists() && file.canRead() && file.isFile() ) {
            stream = new java::FileInputStream(fileName);
        }
    }
    delete[] command;

    if ( stream == nullptr ) {
        Error::error(nullptr, "Can't open file '%s' for %s", fileName, modeToLogAction(StreamOpenMode::READ));
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
    const int pipeFlag = buildPipeCommand(fileName, StreamOpenMode::WRITE, command, commandLength);

    java::OutputStream *stream = nullptr;
    if ( pipeFlag != 0 ) {
        stream = openPipeOutputStream(command);
    } else {
        java::File file(fileName);
        if ( file.canWrite() && !file.isDirectory() ) {
            stream = new java::FileOutputStream(fileName);
        }
    }
    delete[] command;

    if ( stream == nullptr ) {
        Error::error(nullptr, "Can't open file '%s' for %s", fileName, modeToLogAction(StreamOpenMode::WRITE));
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
