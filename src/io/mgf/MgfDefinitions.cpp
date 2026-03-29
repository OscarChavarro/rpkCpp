#include <cstring>

#include "java/io/FileInputStream.h"
#include "common/Error.h"
#include "io/wrapper/FileUncompressWrapper.h"
#include "io/context/LookUpEntity.h"
#include "io/mgf/MgfDefinitions.h"

static const char *
standardInputPath() {
#if defined(_WIN32)
    return "CONIN$";
#else
    return "/dev/stdin";
#endif
}

static bool
skipLines(java::io::InputStream *inputStream, int lineCount) {
    if ( inputStream == nullptr || lineCount < 0 ) {
        return false;
    }
    for ( int line = 0; line < lineCount; line++ ) {
        bool foundEol = false;
        while ( true ) {
            const int ch = inputStream->read();
            if ( ch < 0 ) {
                return false;
            }
            if ( ch == '\n' ) {
                foundEol = true;
                break;
            }
        }
        if ( !foundEol ) {
            return false;
        }
    }
    return true;
}

/**
Default handler for unknown entities
*/
static int
mgfDefaultHandlerForUnknownEntities(int /*ac*/, const char ** /*av*/, const MgfParseSession * /*context*/) {
    // Just ignore line
    return ErrorCodeContext::MGF_OK;
}

void
doError(const char *errmsg, MgfParseSession *context) {
    Error::error(nullptr, "%s line %d: %s", context->readerContext->fileName, context->readerContext->lineNumber, errmsg);
}

void
doWarning(const char *errmsg, MgfParseSession *context) {
    Error::warning(nullptr, "%s line %d: %s", context->readerContext->fileName, context->readerContext->lineNumber, errmsg);
}

/**
Get current position in input file
*/
void
mgfGetFilePosition(FilePositionContext *pos, MgfParseSession *context) {
    pos->fileId = context->readerContext->fileContextId;
    pos->lineNumber = context->readerContext->lineNumber;
    pos->offset = -1;
}

/**
Reposition input file pointer
*/
int
mgfGoToFilePosition(const FilePositionContext *pos, MgfParseSession *context) {
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
    int pipeFlag = 0;
    java::io::InputStream *inputStream = openInputStreamCompressWrapper(context->readerContext->fileName, &pipeFlag);
    if ( inputStream == nullptr || pipeFlag != 0 ) {
        closeInputStream(inputStream);
        return ErrorCodeContext::MGF_ERROR_FILE_SEEK_ERROR;
    }

    if ( !skipLines(inputStream, pos->lineNumber) ) {
        closeInputStream(inputStream);
        return ErrorCodeContext::MGF_ERROR_FILE_SEEK_ERROR;
    }

    context->readerContext->inputStream->dispose();
    delete context->readerContext->inputStream;
    context->readerContext->inputStream = inputStream;
    context->readerContext->lineNumber = pos->lineNumber;
    return ErrorCodeContext::MGF_OK;
}

/**
Get entity number from its name
*/
int
mgfEntity(const char *name, MgfParseSession *context) {
    if ( !context->entityLookUpTable.getCurrentTableSize() ) {
        // Initialize hash table
        if ( !context->entityLookUpTable.lookUpInit(TOTAL_NUMBER_OF_ENTITIES) ) {
            return -1;
        }

        for ( int i = TOTAL_NUMBER_OF_ENTITIES - 1; i >= 0; i-- ) {
            char *entityName = context->entityNames[i];
            context->entityLookUpTable.lookUpFind(entityName)->key = entityName;
        }
    }

    char *entityName = context->entityLookUpTable.lookUpFind(name)->key;
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
mgfHandle(int entityIndex, int argc, const char **argv, MgfParseSession *context) {
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
mgfOpen(ReaderContext *readerContext, const char *functionCallback, MgfParseSession *context) {
    readerContext->fileContextId = ++context->nextFileContextId;
    readerContext->lineNumber = 0;
    readerContext->isPipe = 0;
    readerContext->inputStream = nullptr;
    if ( functionCallback == nullptr ) {
        strcpy(readerContext->fileName, "<stdin>");
        java::io::File standardInputFile(standardInputPath());
        if ( !standardInputFile.exists() || !standardInputFile.canRead() ) {
            return ErrorCodeContext::MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
        }
        java::io::FileInputStream *fileInputStream = new java::io::FileInputStream(standardInputPath());
        readerContext->inputStream = fileInputStream;
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
    readerContext->inputStream = inputStream;

    readerContext->prev = context->readerContext; // Establish new context
    context->readerContext = readerContext;
    return ErrorCodeContext::MGF_OK;
}

/**
Close input file
*/
void
mgfClose(MgfParseSession *context) {
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
mgfLookUpFreeMemory(MgfParseSession *context) {
    if ( context != nullptr ) {
        context->entityLookUpTable.lookUpDone();
    }
}
