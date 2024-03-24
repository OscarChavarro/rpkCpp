#include <cstring>

#include "common/error.h"
#include "io/mgf/mgfDefinitions.h"
#include "io/mgf/lookup.h"
#include "io/mgf/messages.h"
#include "io/FileUncompressWrapper.h"

#define MG_NAMELIST { \
    "#", \
    "c", \
    "cct", \
    "cone", \
    "cmix", \
    "cspec", \
    "cxy", \
    "cyl", \
    "ed", \
    "f", \
    "i", \
    "ies", \
    "ir", \
    "m", \
    "n", \
    "o", \
    "p", \
    "prism", \
    "rd", \
    "ring", \
    "rs", \
    "sides", \
    "sph", \
    "td", \
    "torus", \
    "ts", \
    "v", \
    "xf", \
    "fh" \
}

// Current file context pointer
MgfReaderContext *GLOBAL_mgf_file;

// Count of unknown entities
unsigned GLOBAL_mgf_unknownEntitiesCounter;

// Handler routines for each entity
int (*GLOBAL_mgf_handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv, MgfContext *context);

int (*GLOBAL_mgf_support[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv, MgfContext * /*context*/);

// Error messages
char *GLOBAL_mgf_errors[MGF_NUMBER_OF_ERRORS] = MG_ERROR_LIST;

// Entity names
char GLOBAL_mgf_entityNames[MGF_TOTAL_NUMBER_OF_ENTITIES][MGF_MAXIMUM_ENTITY_NAME_LENGTH] = MG_NAMELIST;

/**
Default handler for unknown entities
*/
int
mgfDefaultHandlerForUnknownEntities(int /*ac*/, char **av)
{
    if ( GLOBAL_mgf_unknownEntitiesCounter++ == 0 ) {
        // Report first incident
        fprintf(stderr, "%s: %d: %s: %s\n", GLOBAL_mgf_file->fileName,
                GLOBAL_mgf_file->lineNumber, GLOBAL_mgf_errors[MGF_ERROR_UNKNOWN_ENTITY], av[0]);
    }
    return MGF_OK;
}

// Handler routine for unknown entities
int (*GLOBAL_mgf_unknownEntityHandleCallback)(int argc, char **argv) = mgfDefaultHandlerForUnknownEntities;

void
doError(const char *errmsg) {
    logError(nullptr, (char *) "%s line %d: %s", GLOBAL_mgf_file->fileName, GLOBAL_mgf_file->lineNumber, errmsg);
}

void
doWarning(const char *errmsg) {
    logWarning(nullptr, (char *) "%s line %d: %s", GLOBAL_mgf_file->fileName, GLOBAL_mgf_file->lineNumber, errmsg);
}

/**
Get current position in input file
*/
void
mgfGetFilePosition(MgdReaderFilePosition *pos)
{
    pos->fid = GLOBAL_mgf_file->fileContextId;
    pos->lineno = GLOBAL_mgf_file->lineNumber;
    pos->offset = ftell(GLOBAL_mgf_file->fp);
}

/**
Reposition input file pointer
*/
int
mgfGoToFilePosition(MgdReaderFilePosition *pos)
{
    if ( pos->fid != GLOBAL_mgf_file->fileContextId ) {
        return MGF_ERROR_FILE_SEEK_ERROR;
    }
    if ( pos->lineno == GLOBAL_mgf_file->lineNumber ) {
        return MGF_OK;
    }
    if ( GLOBAL_mgf_file->fp == stdin || GLOBAL_mgf_file->isPipe ) {
        // Cannot seek on standard input
        return MGF_ERROR_FILE_SEEK_ERROR;
    }
    if ( fseek(GLOBAL_mgf_file->fp, pos->offset, 0) == EOF) {
        return MGF_ERROR_FILE_SEEK_ERROR;
    }
    GLOBAL_mgf_file->lineNumber = pos->lineno;
    return MGF_OK;
}

/**
Get entity number from its name
*/
int
mgfEntity(char *name)
{
    static LUTAB ent_tab = LU_SINIT(nullptr, nullptr); // Lookup table
    char *cp;

    if ( !ent_tab.tsiz ) {
        // Initialize hash table
        if ( !lookUpInit(&ent_tab, MGF_TOTAL_NUMBER_OF_ENTITIES)) {
            return -1;
        }

        // What to do?
        for ( cp = GLOBAL_mgf_entityNames[MGF_TOTAL_NUMBER_OF_ENTITIES - 1];
              cp >= GLOBAL_mgf_entityNames[0];
              cp -= sizeof(GLOBAL_mgf_entityNames[0]) ) {
            lookUpFind(&ent_tab, cp)->key = cp;
        }
    }
    cp = lookUpFind(&ent_tab, name)->key;
    if ( cp == nullptr) {
        return -1;
    }
    return (int)((cp - GLOBAL_mgf_entityNames[0]) / sizeof(GLOBAL_mgf_entityNames[0]));
}

/**
Pass entity to appropriate handler
*/
int
mgfHandle(int en, int ac, char **av, MgfContext *context)
{
    int rv;

    if ( en < 0 && (en = mgfEntity(av[0])) < 0 ) {
        // Unknown entity
        if ( GLOBAL_mgf_unknownEntityHandleCallback != nullptr) {
            return (*GLOBAL_mgf_unknownEntityHandleCallback)(ac, av);
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
mgfOpen(MgfReaderContext *ctx, char *fn)
{
    static int nfids;
    char *cp;
    int isPipe;

    ctx->fileContextId = ++nfids;
    ctx->lineNumber = 0;
    ctx->isPipe = 0;
    if ( fn == nullptr) {
        strcpy(ctx->fileName, "<stdin>");
        ctx->fp = stdin;
        ctx->prev = GLOBAL_mgf_file;
        GLOBAL_mgf_file = ctx;
        return MGF_OK;
    }

    // Get name relative to this context
    if ( GLOBAL_mgf_file != nullptr && (cp = strrchr(GLOBAL_mgf_file->fileName, '/')) != nullptr) {
        strcpy(ctx->fileName, GLOBAL_mgf_file->fileName);
        strcpy(ctx->fileName + (cp - GLOBAL_mgf_file->fileName + 1), fn);
    } else {
        strcpy(ctx->fileName, fn);
    }

    ctx->fp = openFileCompressWrapper(ctx->fileName, "r", &isPipe);
    ctx->isPipe = (char)isPipe;

    if ( ctx->fp == nullptr) {
        return MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
    }

    ctx->prev = GLOBAL_mgf_file; // Establish new context
    GLOBAL_mgf_file = ctx;
    return MGF_OK;
}

/**
Close input file
*/
void
mgfClose()
{
    MgfReaderContext *ctx = GLOBAL_mgf_file;

    GLOBAL_mgf_file = ctx->prev; // Restore enclosing context
    if ( ctx->fp != stdin ) {
        // Close file if it's a file
        closeFile(ctx->fp, ctx->isPipe);
    }
}
