#include <cstring>

#include "common/error.h"
#include "io/FileUncompressWrapper.h"
#include "io/mgf/lookup.h"
#include "io/mgf/MgfReaderFilePosition.h"
#include "io/mgf/mgfDefinitions.h"

static LookUpTable globalLookUpTable = LOOK_UP_INIT(nullptr, nullptr);


/**
Default handler for unknown entities
*/
static int
mgfDefaultHandlerForUnknownEntities(int /*ac*/, const char ** /*av*/, const MgfContext * /*context*/) {
    // Just ignore line
    return MgfErrorCode::MGF_OK;
}

void
doError(const char *errmsg, MgfContext *context) {
    logError(nullptr, "%s line %d: %s", context->readerContext->fileName, context->readerContext->lineNumber, errmsg);
}

void
doWarning(const char *errmsg, MgfContext *context) {
    logWarning(nullptr, "%s line %d: %s", context->readerContext->fileName, context->readerContext->lineNumber, errmsg);
}

/**
Get current position in input file
*/
void
mgfGetFilePosition(MgfReaderFilePosition *pos, MgfContext *context) {
    pos->fid = context->readerContext->fileContextId;
    pos->lineno = context->readerContext->lineNumber;
    pos->offset = ftell(context->readerContext->fp);
}

/**
Reposition input file pointer
*/
int
mgfGoToFilePosition(const MgfReaderFilePosition *pos, MgfContext *context) {
    if ( pos->fid != context->readerContext->fileContextId ) {
        return MgfErrorCode::MGF_ERROR_FILE_SEEK_ERROR;
    }
    if ( pos->lineno == context->readerContext->lineNumber ) {
        return MgfErrorCode::MGF_OK;
    }
    if ( context->readerContext->fp == stdin || context->readerContext->isPipe ) {
        // Cannot seek on standard input
        return MgfErrorCode::MGF_ERROR_FILE_SEEK_ERROR;
    }
    if ( fseek(context->readerContext->fp, pos->offset, 0) == EOF) {
        return MgfErrorCode::MGF_ERROR_FILE_SEEK_ERROR;
    }
    context->readerContext->lineNumber = pos->lineno;
    return MgfErrorCode::MGF_OK;
}

/**
Get entity number from its name
*/
int
mgfEntity(const char *name, MgfContext *context) {
    char *cp;

    if ( !globalLookUpTable.currentTableSize ) {
        // Initialize hash table
        if ( !lookUpInit(&globalLookUpTable, TOTAL_NUMBER_OF_ENTITIES) ) {
            return -1;
        }

        // What to do?
        for ( cp = context->entityNames[TOTAL_NUMBER_OF_ENTITIES - 1];
              cp >= context->entityNames[0];
              cp -= sizeof(context->entityNames[0]) ) {
            lookUpFind(&globalLookUpTable, cp)->key = cp;
        }
    }
    cp = lookUpFind(&globalLookUpTable, name)->key;
    if ( cp == nullptr) {
        return -1;
    }
    return (int)((cp - context->entityNames[0]) / sizeof(context->entityNames[0]));
}

/**
Pass entity to appropriate handler
*/
int
mgfHandle(int entityIndex, int argc, const char **argv, MgfContext *context) {
    entityIndex = mgfEntity(argv[0], context);
    if ( entityIndex < 0 ) {
        // Unknown entity
        return mgfDefaultHandlerForUnknownEntities(argc, argv, context);
    }
    if ( context->supportCallbacks[entityIndex] != nullptr ) {
        // Support handler
        int rv = (*context->supportCallbacks[entityIndex])(argc, argv, context);
        if ( rv != MgfErrorCode::MGF_OK ) {
            return rv;
        }
    }
    return (*context->handleCallbacks[entityIndex])(argc, argv, context); // Assigned handler
}

/**
shaftCullOpen new input file
*/
int
mgfOpen(MgfReaderContext *readerContext, const char *functionCallback, MgfContext *context) {
    static int numberOfFileIds;

    readerContext->fileContextId = ++numberOfFileIds;
    readerContext->lineNumber = 0;
    readerContext->isPipe = 0;
    if ( functionCallback == nullptr ) {
        strcpy(readerContext->fileName, "<stdin>");
        readerContext->fp = stdin;
        readerContext->prev = context->readerContext;
        context->readerContext = readerContext;
        return MgfErrorCode::MGF_OK;
    }

    // Get name relative to this context
    if ( context->readerContext != nullptr ) {
        const char *cp = strrchr(context->readerContext->fileName, '/');
        if ( cp != nullptr ) {
            strcpy(readerContext->fileName, context->readerContext->fileName);
            strcpy(readerContext->fileName + (cp - context->readerContext->fileName + 1), functionCallback);
        }
    } else {
        strcpy(readerContext->fileName, functionCallback);
    }

    int isPipe;
    readerContext->fp = openFileCompressWrapper(readerContext->fileName, "r", &isPipe);
    readerContext->isPipe = (char)isPipe;

    if ( readerContext->fp == nullptr ) {
        return MgfErrorCode::MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
    }

    readerContext->prev = context->readerContext; // Establish new context
    context->readerContext = readerContext;
    return MgfErrorCode::MGF_OK;
}

/**
Close input file
*/
void
mgfClose(MgfContext *context) {
    MgfReaderContext *ctx = context->readerContext;

    context->readerContext = ctx->prev; // Restore enclosing context
    if ( ctx->fp != stdin ) {
        // Close file if it's a file
        closeFile(ctx->fp, ctx->isPipe);
    }
}

void
mgfLookUpFreeMemory() {
    if ( globalLookUpTable.table != nullptr ) {
        delete[] globalLookUpTable.table;
        globalLookUpTable.table = nullptr;
    }
}
