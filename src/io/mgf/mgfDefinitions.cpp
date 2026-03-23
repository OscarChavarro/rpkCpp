#include <cstring>

#include "common/error.h"
#include "java/io/File.h"
#include "java/io/FileInputStream.h"
#include "java/io/BufferedInputStream.h"
#include "io/mgf/lookup.h"
#include "io/mgf/MgfReaderFilePosition.h"
#include "io/mgf/mgfDefinitions.h"

static LookUpTable globalLookUpTable = LOOK_UP_INIT(nullptr, nullptr);

class MgfCallbackHandler final : public MgfEntityHandler {
  public:
    MgfCallbackHandler():
        callback(nullptr)
    {
    }

    explicit MgfCallbackHandler(const HandleCallBack callback):
        callback(callback)
    {
    }

    int
    handle(int argc, const char **argv, MgfContext *context) const override {
        return callback(argc, argv, context);
    }

    bool
    matches(const HandleCallBack candidate) const override {
        return callback == candidate;
    }

  private:
    HandleCallBack callback;
};

static constexpr int MGF_MAXIMUM_CALLBACK_HANDLERS = 128;
static HandleCallBack globalMgfHandlerCallbacks[MGF_MAXIMUM_CALLBACK_HANDLERS];
static MgfCallbackHandler globalMgfHandlers[MGF_MAXIMUM_CALLBACK_HANDLERS];
static int globalMgfHandlerCount = 0;

MgfEntityHandler *
mgfHandlerFromCallback(HandleCallBack callback) {
    if ( callback == nullptr ) {
        return nullptr;
    }
    for ( int i = 0; i < globalMgfHandlerCount; i++ ) {
        if ( globalMgfHandlerCallbacks[i] == callback ) {
            return &globalMgfHandlers[i];
        }
    }
    if ( globalMgfHandlerCount >= MGF_MAXIMUM_CALLBACK_HANDLERS ) {
        logFatal(-1, "mgfHandlerFromCallback", "Too many handler callbacks");
    }
    globalMgfHandlerCallbacks[globalMgfHandlerCount] = callback;
    globalMgfHandlers[globalMgfHandlerCount] = MgfCallbackHandler(callback);
    globalMgfHandlerCount++;
    return &globalMgfHandlers[globalMgfHandlerCount - 1];
}

bool
mgfHandlerMatches(const MgfEntityHandler *handler, HandleCallBack callback) {
    return handler != nullptr && handler->matches(callback);
}


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
mgfGoToFilePosition(const MgfReaderFilePosition *pos, MgfContext *context) {
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
    return static_cast<int>((cp - context->entityNames[0]) / sizeof(context->entityNames[0]));
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
mgfOpen(MgfReaderContext *readerContext, const char *functionCallback, MgfContext *context) {
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
        const char *cp = strrchr(context->readerContext->fileName, '/');
        if ( cp != nullptr ) {
            strcpy(readerContext->fileName, context->readerContext->fileName);
            strcpy(readerContext->fileName + (cp - context->readerContext->fileName + 1), functionCallback);
        }
    } else {
        strcpy(readerContext->fileName, functionCallback);
    }

    java::io::FileInputStream *fileInputStream = new java::io::FileInputStream();
    java::io::File inputFile(readerContext->fileName);
    const bool opened = fileInputStream->open(inputFile);
    inputFile.dispose();
    if ( !opened ) {
        fileInputStream->dispose();
        delete fileInputStream;
        return MgfErrorCode::MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
    }
    readerContext->isPipe = static_cast<char>(fileInputStream->isPipeInput());
    readerContext->inputStream = new java::io::BufferedInputStream(fileInputStream);

    readerContext->prev = context->readerContext; // Establish new context
    context->readerContext = readerContext;
    return MgfErrorCode::MGF_OK;
}

/**
Close input file
*/
void
mgfClose(MgfContext *context) {
    if ( context == nullptr || context->readerContext == nullptr ) {
        return;
    }
    MgfReaderContext *ctx = context->readerContext;

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
    if ( globalLookUpTable.table != nullptr ) {
        delete[] globalLookUpTable.table;
        globalLookUpTable.table = nullptr;
    }
}
