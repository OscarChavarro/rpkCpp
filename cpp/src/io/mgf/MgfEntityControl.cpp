#include <cstring>

#include "java/io/FileInputStream.h"
#include "common/Error.h"
#include "io/wrapper/FileUncompressWrapper.h"
#include "common/dataStructures/LookUpEntity.h"
#include "io/mgf/MgfEntityControl.h"

const char *
MgfEntityControl::standardInputPath() {
    return "/dev/stdin";
}

bool
MgfEntityControl::skipLines(java::InputStream *inputStream, int lineCount) {
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
int
MgfEntityControl::mgfDefaultHandlerForUnknownEntities(int /*ac*/, const char ** /*av*/, const ParseRuntimeContext * /*context*/) {
    // Just ignore line
    return ParseErrorContext::MGF_OK;
}

void
MgfEntityControl::doError(const char *errmsg, ParseRuntimeContext *context) {
    Error::error(nullptr, "%s line %d: %s", context->readerContext->fileName, context->readerContext->lineNumber, errmsg);
}

void
MgfEntityControl::doWarning(const char *errmsg, ParseRuntimeContext *context) {
    Error::warning(nullptr, "%s line %d: %s", context->readerContext->fileName, context->readerContext->lineNumber, errmsg);
}

/**
Get current position in input file
*/
void
MgfEntityControl::mgfGetFilePosition(FilePositionContext *pos, ParseRuntimeContext *context) {
    pos->fileId = context->readerContext->fileContextId;
    pos->lineNumber = context->readerContext->lineNumber;
    pos->offset = -1;
}

/**
Reposition input file pointer
*/
int
MgfEntityControl::mgfGoToFilePosition(const FilePositionContext *pos, ParseRuntimeContext *context) {
    if ( pos->fileId != context->readerContext->fileContextId ) {
        return ParseErrorContext::MGF_ERROR_FILE_SEEK_ERROR;
    }
    if ( pos->lineNumber == context->readerContext->lineNumber ) {
        return ParseErrorContext::MGF_OK;
    }
    if ( context->readerContext->inputStream == nullptr ) {
        return ParseErrorContext::MGF_ERROR_FILE_SEEK_ERROR;
    }
    if ( strcmp(context->readerContext->fileName, "<stdin>") == 0 || context->readerContext->isPipe ) {
        // Cannot seek on standard input or pipes
        return ParseErrorContext::MGF_ERROR_FILE_SEEK_ERROR;
    }
    int pipeFlag = 0;
    java::InputStream *inputStream = FileUncompressWrapper::openInputStreamCompressWrapper(context->readerContext->fileName, &pipeFlag);
    if ( inputStream == nullptr || pipeFlag != 0 ) {
        FileUncompressWrapper::closeInputStream(inputStream);
        return ParseErrorContext::MGF_ERROR_FILE_SEEK_ERROR;
    }

    if ( !MgfEntityControl::skipLines(inputStream, pos->lineNumber) ) {
        FileUncompressWrapper::closeInputStream(inputStream);
        return ParseErrorContext::MGF_ERROR_FILE_SEEK_ERROR;
    }

    context->readerContext->inputStream->dispose();
    delete context->readerContext->inputStream;
    context->readerContext->inputStream = inputStream;
    context->readerContext->lineNumber = pos->lineNumber;
    return ParseErrorContext::MGF_OK;
}

/**
Get entity number from its name
*/
int
MgfEntityControl::mgfEntity(const char *name, ParseRuntimeContext *context) {
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
MgfEntityControl::mgfHandle(int entityIndex, int argc, const char **argv, ParseRuntimeContext *context) {
    entityIndex = MgfEntityControl::mgfEntity(argv[0], context);
    if ( entityIndex < 0 ) {
        // Unknown entity
        return MgfEntityControl::mgfDefaultHandlerForUnknownEntities(argc, argv, context);
    }
    if ( context->readerStackState.supportCallbacks[entityIndex] != nullptr ) {
        // Support handler
        int rv = context->readerStackState.supportCallbacks[entityIndex]->handle(argc, argv, context);
        if ( rv != ParseErrorContext::MGF_OK ) {
            return rv;
        }
    }
    return context->readerStackState.handleCallbacks[entityIndex]->handle(argc, argv, context); // Assigned handler
}

/**
shaftCullOpen new input file
*/
int
MgfEntityControl::mgfOpen(ReaderContext *readerContext, const char *functionCallback, ParseRuntimeContext *context) {
    readerContext->fileContextId = ++context->nextFileContextId;
    readerContext->lineNumber = 0;
    readerContext->isPipe = 0;
    readerContext->inputStream = nullptr;
    if ( functionCallback == nullptr ) {
        strcpy(readerContext->fileName, "<stdin>");
        java::File standardInputFile(standardInputPath());
        if ( !standardInputFile.exists() || !standardInputFile.canRead() ) {
            return ParseErrorContext::MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
        }
        java::FileInputStream *fileInputStream = new java::FileInputStream(standardInputPath());
        readerContext->inputStream = fileInputStream;
        readerContext->prev = context->readerContext;
        context->readerContext = readerContext;
        return ParseErrorContext::MGF_OK;
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
    java::InputStream *inputStream = FileUncompressWrapper::openInputStreamCompressWrapper(readerContext->fileName, &pipeFlag);
    if ( inputStream == nullptr ) {
        return ParseErrorContext::MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
    }
    readerContext->isPipe = static_cast<char>(pipeFlag != 0);
    readerContext->inputStream = inputStream;

    readerContext->prev = context->readerContext; // Establish new context
    context->readerContext = readerContext;
    return ParseErrorContext::MGF_OK;
}

/**
Close input file
*/
void
MgfEntityControl::mgfClose(ParseRuntimeContext *context) {
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
MgfEntityControl::mgfLookUpFreeMemory(ParseRuntimeContext *context) {
    if ( context != nullptr ) {
        context->entityLookUpTable.lookUpDone();
    }
}
