#include <cstring>

#include "common/error.h"
#include "io/mgf/mgfDefinitions.h"
#include "io/mgf/lookup.h"
#include "io/FileUncompressWrapper.h"

// Count of unknown entities
unsigned GLOBAL_mgf_unknownEntitiesCounter;

// Handler routines for each entity
int (*GLOBAL_mgf_handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv, MgfContext *context);
int (*GLOBAL_mgf_support[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv, MgfContext * /*context*/);

/**
Default handler for unknown entities
*/
int
mgfDefaultHandlerForUnknownEntities(int /*ac*/, char **av, MgfContext *context)
{
    if ( GLOBAL_mgf_unknownEntitiesCounter++ == 0 ) {
        // Report first incident
        fprintf(stderr, "%s: %d: %s: %s\n", context->readerContext->fileName,
                context->readerContext->lineNumber, context->errorCodeMessages[MGF_ERROR_UNKNOWN_ENTITY], av[0]);
    }
    return MGF_OK;
}

// Handler routine for unknown entities
int (*GLOBAL_mgf_unknownEntityHandleCallback)(int argc, char **argv, MgfContext *context) = mgfDefaultHandlerForUnknownEntities;

void
doError(const char *errmsg, MgfContext *context) {
    logError(nullptr, (char *) "%s line %d: %s", context->readerContext->fileName, context->readerContext->lineNumber, errmsg);
}

void
doWarning(const char *errmsg, MgfContext *context) {
    logWarning(nullptr, (char *) "%s line %d: %s", context->readerContext->fileName, context->readerContext->lineNumber, errmsg);
}

/**
Get current position in input file
*/
void
mgfGetFilePosition(MgdReaderFilePosition *pos, MgfContext *context)
{
    pos->fid = context->readerContext->fileContextId;
    pos->lineno = context->readerContext->lineNumber;
    pos->offset = ftell(context->readerContext->fp);
}

/**
Reposition input file pointer
*/
int
mgfGoToFilePosition(MgdReaderFilePosition *pos, MgfContext *context)
{
    if ( pos->fid != context->readerContext->fileContextId ) {
        return MGF_ERROR_FILE_SEEK_ERROR;
    }
    if ( pos->lineno == context->readerContext->lineNumber ) {
        return MGF_OK;
    }
    if ( context->readerContext->fp == stdin || context->readerContext->isPipe ) {
        // Cannot seek on standard input
        return MGF_ERROR_FILE_SEEK_ERROR;
    }
    if ( fseek(context->readerContext->fp, pos->offset, 0) == EOF) {
        return MGF_ERROR_FILE_SEEK_ERROR;
    }
    context->readerContext->lineNumber = pos->lineno;
    return MGF_OK;
}

/**
Get entity number from its name
*/
int
mgfEntity(char *name, MgfContext *context)
{
    static LookUpTable ent_tab = LOOK_UP_INIT(nullptr, nullptr); // Lookup table
    char *cp;

    if ( !ent_tab.currentTableSize ) {
        // Initialize hash table
        if ( !lookUpInit(&ent_tab, MGF_TOTAL_NUMBER_OF_ENTITIES)) {
            return -1;
        }

        // What to do?
        for ( cp = context->entityNames[MGF_TOTAL_NUMBER_OF_ENTITIES - 1];
              cp >= context->entityNames[0];
              cp -= sizeof(context->entityNames[0]) ) {
            lookUpFind(&ent_tab, cp)->key = cp;
        }
    }
    cp = lookUpFind(&ent_tab, name)->key;
    if ( cp == nullptr) {
        return -1;
    }
    return (int)((cp - context->entityNames[0]) / sizeof(context->entityNames[0]));
}

/**
Pass entity to appropriate handler
*/
int
mgfHandle(int en, int ac, char **av, MgfContext *context)
{
    int rv;

    if ( en < 0 && (en = mgfEntity(av[0], context)) < 0 ) {
        // Unknown entity
        if ( GLOBAL_mgf_unknownEntityHandleCallback != nullptr) {
            return (*GLOBAL_mgf_unknownEntityHandleCallback)(ac, av, context);
        }
        return MGF_ERROR_UNKNOWN_ENTITY;
    }
    if ( GLOBAL_mgf_support[en] != nullptr) {
        // Support handler
        // TODO SITHMASTER: Check number of arguments here
        rv = (*GLOBAL_mgf_support[en])(ac, av, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
    }
    return (*GLOBAL_mgf_handleCallbacks[en])(ac, av, context); // Assigned handler
}

/**
shaftCullOpen new input file
*/
int
mgfOpen(MgfReaderContext *readerContext, char *functionCallback, MgfContext *context)
{
    static int numberOfFileIds;
    char *cp;
    int isPipe;

    readerContext->fileContextId = ++numberOfFileIds;
    readerContext->lineNumber = 0;
    readerContext->isPipe = 0;
    if ( functionCallback == nullptr ) {
        strcpy(readerContext->fileName, "<stdin>");
        readerContext->fp = stdin;
        readerContext->prev = context->readerContext;
        context->readerContext = readerContext;
        return MGF_OK;
    }

    // Get name relative to this context
    if ( context->readerContext != nullptr && (cp = strrchr(context->readerContext->fileName, '/')) != nullptr ) {
        strcpy(readerContext->fileName, context->readerContext->fileName);
        strcpy(readerContext->fileName + (cp - context->readerContext->fileName + 1), functionCallback);
    } else {
        strcpy(readerContext->fileName, functionCallback);
    }

    readerContext->fp = openFileCompressWrapper(readerContext->fileName, "r", &isPipe);
    readerContext->isPipe = (char)isPipe;

    if ( readerContext->fp == nullptr ) {
        return MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
    }

    readerContext->prev = context->readerContext; // Establish new context
    context->readerContext = readerContext;
    return MGF_OK;
}

/**
Close input file
*/
void
mgfClose(MgfContext *context)
{
    MgfReaderContext *ctx = context->readerContext;

    context->readerContext = ctx->prev; // Restore enclosing context
    if ( ctx->fp != stdin ) {
        // Close file if it's a file
        closeFile(ctx->fp, ctx->isPipe);
    }
}
