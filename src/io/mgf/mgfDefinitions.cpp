#include <cstring>

#include "common/error.h"
#include "java/io/FileInputStream.h"
#include "java/io/BufferedInputStream.h"
#include "io/wrapper/FileUncompressWrapper.h"
#include "io/context/LookUpEntity.h"
#include "io/context/FilePositionContext.h"
#include "io/mgf/mgfDefinitions.h"

static LookUpTable globalLookUpTable(LookUpBehaviors::nonOwningCString());

static const char *
standardInputPath() {
#if defined(_WIN32)
    return "CONIN$";
#else
    return "/dev/stdin";
#endif
}

/**
Default handler for unknown entities
*/
static int
mgfDefaultHandlerForUnknownEntities(int /*ac*/, const char ** /*av*/, const BaseContext * /*context*/) {
    // Just ignore line
    return ErrorCodeContext::MGF_OK;
}

void
doError(const char *errmsg, BaseContext *context) {
    logError(nullptr, "%s line %d: %s", context->readerContext->fileName, context->readerContext->lineNumber, errmsg);
}

void
doWarning(const char *errmsg, BaseContext *context) {
    logWarning(nullptr, "%s line %d: %s", context->readerContext->fileName, context->readerContext->lineNumber, errmsg);
}

/**
Get current position in input file
*/
void
mgfGetFilePosition(FilePositionContext *pos, BaseContext *context) {
    pos->fileId = context->readerContext->fileContextId;
    pos->lineNumber = context->readerContext->lineNumber;
    if ( context->readerContext->inputStream == nullptr ) {
        pos->offset = -1;
    } else {
        pos->offset = context->readerContext->inputStream->tell();
    }
}

/**
Reposition input file pointer
*/
int
mgfGoToFilePosition(const FilePositionContext *pos, BaseContext *context) {
    if ( pos->fileId != context->readerContext->fileContextId ) {
        return ErrorCodeContext::MGF_ERROR_FILE_SEEK_ERROR;
    }
    if ( pos->lineNumber == context->readerContext->lineNumber ) {
        return ErrorCodeContext::MGF_OK;
    }
    if ( context->readerContext->inputStream == nullptr ) {
        return ErrorCodeContext::MGF_ERROR_FILE_SEEK_ERROR;
    }
    if ( strcmp(context->readerContext->fileName, "<stdin>") == 0 || context->readerContext->isPipe ) {
        // Cannot seek on standard input or pipes
        return ErrorCodeContext::MGF_ERROR_FILE_SEEK_ERROR;
    }
    if ( pos->offset < 0 ) {
        return ErrorCodeContext::MGF_ERROR_FILE_SEEK_ERROR;
    }

    int pipeFlag = 0;
    java::io::InputStream *inputStream = openInputStreamCompressWrapper(context->readerContext->fileName, &pipeFlag);
    if ( inputStream == nullptr || pipeFlag != 0 ) {
        closeInputStream(inputStream);
        return ErrorCodeContext::MGF_ERROR_FILE_SEEK_ERROR;
    }

    java::io::BufferedInputStream *newInputStream =
        new java::io::BufferedInputStream(inputStream, true);
    if ( !newInputStream->isOpen() ) {
        newInputStream->dispose();
        delete newInputStream;
        return ErrorCodeContext::MGF_ERROR_FILE_SEEK_ERROR;
    }

    long remaining = pos->offset;
    unsigned char buffer[512];
    while ( remaining > 0 ) {
        const int chunk = remaining > static_cast<long>(sizeof(buffer))
            ? static_cast<int>(sizeof(buffer))
            : static_cast<int>(remaining);
        const int readCount = newInputStream->read(buffer, 0, chunk);
        if ( readCount <= 0 ) {
            newInputStream->dispose();
            delete newInputStream;
            return ErrorCodeContext::MGF_ERROR_FILE_SEEK_ERROR;
        }
        remaining -= readCount;
    }

    context->readerContext->inputStream->dispose();
    delete context->readerContext->inputStream;
    context->readerContext->inputStream = newInputStream;
    context->readerContext->lineNumber = pos->lineNumber;
    return ErrorCodeContext::MGF_OK;
}

/**
Get entity number from its name
*/
int
mgfEntity(const char *name, BaseContext *context) {
    if ( !globalLookUpTable.getCurrentTableSize() ) {
        // Initialize hash table
        if ( !globalLookUpTable.lookUpInit(TOTAL_NUMBER_OF_ENTITIES) ) {
            return -1;
        }

        for ( int i = TOTAL_NUMBER_OF_ENTITIES - 1; i >= 0; i-- ) {
            char *entityName = context->entityNames[i];
            globalLookUpTable.lookUpFind(entityName)->key = entityName;
        }
    }

    char *entityName = globalLookUpTable.lookUpFind(name)->key;
    if ( entityName == nullptr) {
        return -1;
    }
    for ( int i = 0; i < TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( context->entityNames[i] == entityName ) {
            return i;
        }
    }
    return -1;
}

/**
Pass entity to appropriate handler
*/
int
mgfHandle(int entityIndex, int argc, const char **argv, BaseContext *context) {
    entityIndex = mgfEntity(argv[0], context);
    if ( entityIndex < 0 ) {
        // Unknown entity
        return mgfDefaultHandlerForUnknownEntities(argc, argv, context);
    }
    if ( context->supportCallbacks[entityIndex] != nullptr ) {
        // Support handler
        int rv = context->supportCallbacks[entityIndex]->handle(argc, argv, context);
        if ( rv != ErrorCodeContext::MGF_OK ) {
            return rv;
        }
    }
    return context->handleCallbacks[entityIndex]->handle(argc, argv, context); // Assigned handler
}

/**
shaftCullOpen new input file
*/
int
mgfOpen(ReaderContext *readerContext, const char *functionCallback, BaseContext *context) {
    static int numberOfFileIds;

    readerContext->fileContextId = ++numberOfFileIds;
    readerContext->lineNumber = 0;
    readerContext->isPipe = 0;
    readerContext->inputStream = nullptr;
    if ( functionCallback == nullptr ) {
        strcpy(readerContext->fileName, "<stdin>");
        java::io::FileInputStream *fileInputStream = new java::io::FileInputStream(standardInputPath());
        if ( !fileInputStream->isOpen() ) {
            fileInputStream->dispose();
            delete fileInputStream;
            return ErrorCodeContext::MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
        }
        readerContext->inputStream = new java::io::BufferedInputStream(fileInputStream);
        readerContext->prev = context->readerContext;
        context->readerContext = readerContext;
        return ErrorCodeContext::MGF_OK;
    }

    // Get name relative to this context
    if ( context->readerContext != nullptr ) {
        const char *currentFileName = context->readerContext->fileName;
        int slashIndex = -1;
        for ( int i = 0; currentFileName[i] != '\0'; i++ ) {
            if ( currentFileName[i] == '/' ) {
                slashIndex = i;
            }
        }
        if ( slashIndex >= 0 ) {
            strcpy(readerContext->fileName, context->readerContext->fileName);
            strcpy(&readerContext->fileName[slashIndex + 1], functionCallback);
        } else {
            strcpy(readerContext->fileName, functionCallback);
        }
    } else {
        strcpy(readerContext->fileName, functionCallback);
    }

    int pipeFlag = false;
    java::io::InputStream *inputStream = openInputStreamCompressWrapper(readerContext->fileName, &pipeFlag);
    if ( inputStream == nullptr ) {
        return ErrorCodeContext::MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
    }
    readerContext->isPipe = static_cast<char>(pipeFlag != 0);
    readerContext->inputStream = new java::io::BufferedInputStream(inputStream, true);
    if ( !readerContext->inputStream->isOpen() ) {
        readerContext->inputStream->dispose();
        delete readerContext->inputStream;
        readerContext->inputStream = nullptr;
        return ErrorCodeContext::MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
    }

    readerContext->prev = context->readerContext; // Establish new context
    context->readerContext = readerContext;
    return ErrorCodeContext::MGF_OK;
}

/**
Close input file
*/
void
mgfClose(BaseContext *context) {
    if ( context == nullptr || context->readerContext == nullptr ) {
        return;
    }
    ReaderContext *ctx = context->readerContext;

    context->readerContext = ctx->prev; // Restore enclosing context
    if ( ctx->inputStream != nullptr ) {
        // Close file if it's a file
        ctx->inputStream->dispose();
        delete ctx->inputStream;
        ctx->inputStream = nullptr;
    }
}

void
mgfLookUpFreeMemory() {
    globalLookUpTable.lookUpDone();
}
