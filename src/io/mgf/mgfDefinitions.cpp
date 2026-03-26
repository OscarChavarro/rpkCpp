#include <cstring>

#include "common/error.h"
#include "java/io/FileInputStream.h"
#include "java/io/BufferedInputStream.h"
#include "io/FileUncompressWrapper.h"
#include "io/mgf/LookUpEntity.h"
#include "io/mgf/MgfReaderFilePosition.h"
#include "io/mgf/mgfDefinitions.h"

static LookUpTable globalLookUpTable(LookUpBehaviors::nonOwningCString());

/**
Default handler for unknown entities
*/
static int
mgfDefaultHandlerForUnknownEntities(int /*ac*/, const char ** /*av*/, const BaseContext * /*context*/) {
    // Just ignore line
    return MgfErrorCode::MGF_OK;
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
mgfGetFilePosition(MgfReaderFilePosition *pos, BaseContext *context) {
    pos->fid = context->readerContext->fileContextId;
    pos->lineno = context->readerContext->lineNumber;
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
mgfGoToFilePosition(const MgfReaderFilePosition *pos, BaseContext *context) {
    if ( pos->fid != context->readerContext->fileContextId ) {
        return MgfErrorCode::MGF_ERROR_FILE_SEEK_ERROR;
    }
    if ( pos->lineno == context->readerContext->lineNumber ) {
        return MgfErrorCode::MGF_OK;
    }
    if ( context->readerContext->inputStream == nullptr ) {
        return MgfErrorCode::MGF_ERROR_FILE_SEEK_ERROR;
    }
    if ( context->readerContext->inputStream->isStandardInput() || context->readerContext->isPipe ) {
        // Cannot seek on standard input
        return MgfErrorCode::MGF_ERROR_FILE_SEEK_ERROR;
    }
    if ( !context->readerContext->inputStream->seek(pos->offset) ) {
        return MgfErrorCode::MGF_ERROR_FILE_SEEK_ERROR;
    }
    context->readerContext->lineNumber = pos->lineno;
    return MgfErrorCode::MGF_OK;
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
        if ( rv != MgfErrorCode::MGF_OK ) {
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
        java::io::FileInputStream *fileInputStream = new java::io::FileInputStream();
        if ( !fileInputStream->openStandardInput() ) {
            fileInputStream->dispose();
            delete fileInputStream;
            return MgfErrorCode::MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
        }
        readerContext->inputStream = new java::io::BufferedInputStream(fileInputStream);
        readerContext->prev = context->readerContext;
        context->readerContext = readerContext;
        return MgfErrorCode::MGF_OK;
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

    java::io::FileInputStream *fileInputStream = new java::io::FileInputStream();
    int pipeFlag = false;
    FILE *inputHandle = openFileCompressWrapper(readerContext->fileName, "r", &pipeFlag);
    if ( !fileInputStream->open(inputHandle, pipeFlag != 0) ) {
        fileInputStream->dispose();
        delete fileInputStream;
        return MgfErrorCode::MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
    }
    readerContext->isPipe = static_cast<char>(pipeFlag != 0);
    readerContext->inputStream = new java::io::BufferedInputStream(fileInputStream);

    readerContext->prev = context->readerContext; // Establish new context
    context->readerContext = readerContext;
    return MgfErrorCode::MGF_OK;
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
