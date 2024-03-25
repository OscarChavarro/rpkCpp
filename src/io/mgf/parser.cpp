/**
Parse an mgf file, converting or discarding unsupported entities.
*/

#include <cctype>
#include <cstring>

#include "io/FileUncompressWrapper.h"
#include "io/mgf/MgfTransformContext.h"
#include "io/mgf/parser.h"
#include "io/mgf/mgfHandlerGeometry.h"
#include "io/mgf/mgfHandlerMaterial.h"

/**
The idea with this parser is to compensate for any missing entries in
handleCallbacks with alternate handlers that express these entities in terms
of others that the calling program can handle.

In some cases, no alternate handler is possible because the entity
has no approximate equivalent.  These entities are simply discarded.

Certain entities are dependent on others, and mgfAlternativeInit() will fail
if the supported entities are not consistent.

Some alternate entity handlers require that earlier entities be
noted in some fashion, and we therefore keep another array of
parallel support handlers to assist in this effort.
*/

/**
Read next line from file
*/
int
mgfReadNextLine(MgfContext *context)
{
    int len = 0;

    do {
        if ( fgets(context->readerContext->inputLine + len,
                   MGF_MAXIMUM_INPUT_LINE_LENGTH - len, context->readerContext->fp) == nullptr) {
            return len;
        }
        len += (int)strlen(context->readerContext->inputLine + len);
        if ( len >= MGF_MAXIMUM_INPUT_LINE_LENGTH - 1 ) {
            return len;
        }
        context->readerContext->lineNumber++;
    } while ( len > 1 && context->readerContext->inputLine[len - 2] == '\\' );

    return len;
}

/**
Parse current input line
*/
int
mgfParseCurrentLine(MgfContext *context)
{
    char buffer[MGF_MAXIMUM_INPUT_LINE_LENGTH];
    char *argv[MGF_MAXIMUM_ARGUMENT_COUNT];
    char *cp;
    char *cp2;
    char **ap;

    // Copy line, removing escape chars
    cp = buffer;
    cp2 = context->readerContext->inputLine;
    while ((*cp++ = *cp2++)) {
        if ( cp2[0] == '\n' && cp2[-1] == '\\' ) {
            cp--;
        }
    }
    cp = buffer;
    ap = argv; // Break into words
    for ( ;; ) {
        while ( isspace(*cp)) {
            *cp++ = '\0';
        }
        if ( !*cp ) {
            break;
        }
        if ( ap - argv >= MGF_MAXIMUM_ARGUMENT_COUNT - 1 ) {
            return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        *ap++ = cp;
        while ( *++cp && !isspace(*cp) );
    }
    if ( ap == argv ) {
        // No words in line
        return MGF_OK;
    }
    *ap = nullptr;
    // Else handle it
    return mgfHandle(-1, (int)(ap - argv), argv, context);
}

/**
Clear parser history
*/
void
mgfClear(MgfContext *context)
{
    initColorContextTables();
    initGeometryContextTables(context);
    initMaterialContextTables();
    while ( context->readerContext != nullptr) {
        // Reset our file context
        mgfClose(context);
    }
}
