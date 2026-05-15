#include <string.h>

#include "java/io/FileInputStream.h"
#include "java/io/FileOutputStream.h"
#include "java/util/Formatter.h"
#include "common/logging/Logger.h"
#include "io/wrapper/FileUncompressWrapper.h"
#include "io/wrapper/PipeInputStream.h"
#include "io/wrapper/PipeOutputStream.h"
#include "io/wrapper/StreamOpenMode.h"

const char *
FileUncompressWrapper::modeToLogAction(StreamOpenMode mode) {
    return mode == READ ? "reading" : "writing";
}

bool
FileUncompressWrapper::isInvalidFileName(const char *fileName) {
    if ( fileName == NULL || fileName[0] == '\0' || fileName[strlen(fileName) - 1] == '/' ) {
        return true;
    }
    return false;
}

bool
FileUncompressWrapper::buildPipeCommand(const char *fileName, StreamOpenMode openMode, char *command, int commandLength) {
    if ( fileName == NULL || command == NULL || commandLength <= 0 ) {
        return false;
    }

    if ( fileName[0] == '|' ) {
        Formatter::format(command, commandLength, "%s", &fileName[1]);
        return true;
    }

    const char *ext = strrchr(fileName, '.');
    if ( ext && strcmp(ext, ".gz") == 0 ) {
        if ( openMode == READ ) {
            Formatter::format(command, commandLength, "gunzip < %s", fileName);
        } else {
            Formatter::format(command, commandLength, "gzip > %s", fileName);
        }
    } else if ( ext && strcmp(ext, ".Z") == 0 ) {
        if ( openMode == READ ) {
            Formatter::format(command, commandLength, "uncompress < %s", fileName);
        } else {
            Formatter::format(command, commandLength, "compress > %s", fileName);
        }
    } else if ( ext && strcmp(ext, ".bz") == 0 ) {
        if ( openMode == READ ) {
            Formatter::format(command, commandLength, "bunzip < %s", fileName);
        } else {
            Formatter::format(command, commandLength, "bzip > %s", fileName);
        }
    } else if ( ext && strcmp(ext, ".bz2") == 0 ) {
        if ( openMode == READ ) {
            Formatter::format(command, commandLength, "bunzip2 < %s", fileName);
        } else {
            Formatter::format(command, commandLength, "bzip2 > %s", fileName);
        }
    } else {
        return false;
    }
    return true;
}

InputStream *
FileUncompressWrapper::openPipeInputStream(const char *command) {
    PipeInputStream *pipeStream = new PipeInputStream(command);
    if ( !pipeStream->isOpen() ) {
        delete pipeStream;
        return NULL;
    }
    return pipeStream;
}

OutputStream *
FileUncompressWrapper::openPipeOutputStream(const char *command) {
    PipeOutputStream *pipeStream = new PipeOutputStream(command);
    if ( !pipeStream->isOpen() ) {
        delete pipeStream;
        return NULL;
    }
    return pipeStream;
}

InputStream *
FileUncompressWrapper::openInputStreamCompressWrapper(const char *fileName, int *isPipe) {
    if ( isPipe != NULL ) {
        *isPipe = 0;
    }
    if ( isInvalidFileName(fileName) ) {
        return NULL;
    }

    const int commandLength = ((int)(strlen(fileName))) + 20;
    char *command = new char[commandLength];
    const bool pipeFlag = buildPipeCommand(fileName, READ, command, commandLength);

    InputStream *stream = NULL;
    if ( pipeFlag ) {
        stream = openPipeInputStream(command);
    } else {
        File file(fileName);
        if ( file.exists() && file.canRead() && file.isFile() ) {
            stream = new FileInputStream(fileName);
        }
    }
    delete[] command;

    if ( stream == NULL ) {
        Logger::error(NULL, "Can't open file '%s' for %s", fileName, modeToLogAction(READ));
        if ( isPipe != NULL ) {
            *isPipe = 0;
        }
        return NULL;
    }

    if ( isPipe != NULL ) {
        *isPipe = pipeFlag ? 1 : 0;
    }
    return stream;
}

OutputStream *
FileUncompressWrapper::openOutputStreamCompressWrapper(const char *fileName, int *isPipe) {
    if ( isPipe != NULL ) {
        *isPipe = 0;
    }
    if ( isInvalidFileName(fileName) ) {
        return NULL;
    }

    const int commandLength = ((int)(strlen(fileName))) + 20;
    char *command = new char[commandLength];
    const bool pipeFlag = buildPipeCommand(fileName, WRITE, command, commandLength);

    OutputStream *stream = NULL;
    if ( pipeFlag ) {
        stream = openPipeOutputStream(command);
    } else {
        File file(fileName);
        if ( file.canWrite() && !file.isDirectory() ) {
            stream = new FileOutputStream(fileName);
        }
    }
    delete[] command;

    if ( stream == NULL ) {
        Logger::error(NULL, "Can't open file '%s' for %s", fileName, modeToLogAction(WRITE));
        if ( isPipe != NULL ) {
            *isPipe = 0;
        }
        return NULL;
    }

    if ( isPipe != NULL ) {
        *isPipe = pipeFlag ? 1 : 0;
    }
    return stream;
}

void
FileUncompressWrapper::closeInputStream(InputStream *stream) {
    if ( stream == NULL ) {
        return;
    }
    stream->close();
    delete stream;
}

void
FileUncompressWrapper::closeOutputStream(OutputStream *stream) {
    if ( stream == NULL ) {
        return;
    }
    stream->close();
    delete stream;
}
