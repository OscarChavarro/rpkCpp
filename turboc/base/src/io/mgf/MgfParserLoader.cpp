#include <string.h>

#include "java/io/BufferedInputStream.h"
#include "java/lang/StringBuilder.h"
#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "java/util/Formatter.h"
#include "java/util/StringTokenizer.h"
#include "common/logging/Logger.h"
#include "io/mgf/MgfEntityControl.h"
#include "io/mgf/MgfConeEntityTessellator.h"
#include "io/mgf/MgfCylinderEntityExpander.h"
#include "io/mgf/MgfFaceWithHolesEntityExpander.h"
#include "io/mgf/MgfColorEntitySupport.h"
#include "io/mgf/MgfVertexFaceEntitySupport.h"
#include "io/mgf/MgfMaterialEntitySupport.h"
#include "io/mgf/MgfObjectNameSupport.h"
#include "io/mgf/MgfPrismEntityTessellator.h"
#include "io/mgf/MgfRingEntityTessellator.h"
#include "io/mgf/MgfSphereEntityExpander.h"
#include "io/mgf/MgfTransformationSupport.h"
#include "io/mgf/MgfEntityHandlerAdapter.h"
#include "io/mgf/MgfTorusEntityExpander.h"
#include "io/mgf/MgfParserLoader.h"

/**
The parser follows the following process:
1. Fills in the handleCallbacks array with handlers for each entity
   it knows how to handle.
2. Call mgfInit to fill in the rest.  This function will report
   an error and quit if it tries to support an inconsistent set of entities.
3. For each file to parse, call mgfLoad with the file name. To read from
   standard input, use NULL as the file name.

 For additional control over error reporting and file management,
use mgfOpen, mgfReadNextLine, mgfParseCurrentLine and mgfClose instead of mgfLoad.
To pass an entity of your own construction to the parser, use
the mgfHandle function rather than the handleCallbacks routines directly.
(The first argument to mgfHandle is the entity #, or -1.)
To free any data structures and clear the parser, use mgfClear.
If there is an error, mgfLoad, mgfOpen, mgfParseCurrentLine, mgfHandle and
mgfGoToFilePosition will return an error from the list above.  In addition,
mgfLoad will report the error to stderr. The mgfReadNextLine routine
returns 0 when the end of file has been reached.

The idea with this parser is to compensate for any missing entries in
handleCallbacks with alternate handlers that express these entities in terms
of others that the calling program can handle.

In some cases, no alternate handler is possible because the entity
has no approximate equivalent. These entities are simply discarded.

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
MgfParserLoader::readInputLine(InputStream *inputStream, char *readBuffer, int maxLength) {
    if ( inputStream == NULL || readBuffer == NULL || maxLength <= 0 ) {
        return 0;
    }
    int length = 0;
    while ( length < maxLength - 1 ) {
        const int readChar = inputStream->read();
        if ( readChar < 0 ) {
            break;
        }
        readBuffer[length++] = ((char)(readChar));
        if ( readChar == '\n' ) {
            break;
        }
    }
    readBuffer[length] = '\0';
    return length;
}

int
MgfParserLoader::mgfReadNextLine(const ParseRuntimeContext *context) {
    if ( context->readerContext->inputStream == NULL ) {
        return 0;
    }

    int len = 0;
    StringBuilder lineBuilder;
    char readBuffer[ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH];

    do {
        const int maxLength = ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - len;
        if ( maxLength <= 0 ) {
            lineBuilder.dispose();
            return len;
        }
        const int readLength = readInputLine(context->readerContext->inputStream, readBuffer, maxLength);
        if ( readLength <= 0 ) {
            String line = lineBuilder.toString();
            strncpy(context->readerContext->inputLine, line.toCString(), ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1);
            context->readerContext->inputLine[ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1] = '\0';
            line.dispose();
            lineBuilder.dispose();
            return len;
        }

        lineBuilder.append(readBuffer, readLength);
        len = lineBuilder.length();
        if ( len >= ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1 ) {
            String line = lineBuilder.toString();
            strncpy(context->readerContext->inputLine, line.toCString(), ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1);
            context->readerContext->inputLine[ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1] = '\0';
            line.dispose();
            lineBuilder.dispose();
            return len;
        }
        context->readerContext->lineNumber++;
    } while ( len > 1 && lineBuilder.charAt(len - 2) == '\\' );

    String line = lineBuilder.toString();
    strncpy(context->readerContext->inputLine, line.toCString(), ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1);
    context->readerContext->inputLine[ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1] = '\0';
    line.dispose();
    lineBuilder.dispose();

    return len;
}

/**
Parse current input line
*/
int
MgfParserLoader::mgfParseCurrentLine(ParseRuntimeContext *context) {
    const char *argv[ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT];
    String tokens[ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT];
    int argc = 0;

    // Copy line, removing escape chars
    StringBuilder buffer;
    String inputLine(context->readerContext->inputLine);
    for ( int i = 0; i < inputLine.length(); i++ ) {
        const char current = inputLine.charAt(i);
        const char next = inputLine.charAt(i + 1);
        if ( current == '\\' && next == '\n' ) {
            continue;
        }
        buffer.append(current);
    }

    StringTokenizer tokenizer(buffer.toString(), " \t\r\n\f\v");
    while ( tokenizer.hasMoreTokens() ) {
        if ( argc >= ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT - 1 ) {
            for ( int i = 0; i < argc; i++ ) {
                tokens[i].dispose();
            }
            tokenizer.dispose();
            buffer.dispose();
            inputLine.dispose();
            return MGF_ERRR_WRNG_NUM_O_ARGMN;
        }
        tokens[argc] = tokenizer.nextToken();
        argv[argc] = tokens[argc].toCString();
        argc++;
    }
    if ( argc == 0 ) {
        // No words in line
        for ( int i = 0; i < argc; i++ ) {
            tokens[i].dispose();
        }
        tokenizer.dispose();
        buffer.dispose();
        inputLine.dispose();
        return MGF_OK;
    }
    argv[argc] = NULL;
    tokenizer.dispose();
    buffer.dispose();
    inputLine.dispose();
    // Else handle it
    const int status = MgfEntityControl::mgfHandle(-1, argc, argv, context);
    for ( int i = 0; i < argc; i++ ) {
        tokens[i].dispose();
    }
    return status;
}

/**
Clear parser history
*/
void
MgfParserLoader::mgfClear(ParseRuntimeContext *context) {
    MgfColorEntitySupport::initColorContextTables(context);
    MgfVertexFaceEntitySupport::initGeometryContextTables(context);
    MgfMaterialEntitySupport::initMaterialContextTables(context);
    while ( context->readerContext != NULL) {
        // Reset our file context
        MgfEntityControl::mgfClose(context);
    }
}

/**
Sets the number of quarter circle divisions for discrete approximation of cylinders, spheres, cones, etc.
*/
void
MgfParserLoader::mgfSetNrQuartCircDivs(int divs) {
    if ( divs <= 0 ) {
        Logger::error(NULL, "Number of quarter circle divisions (%d) should be positive", divs);
        return;
    }
}

/**
If yesno is true, all materials will be converted to be monochrome
*/
void
MgfParserLoader::mgfSetMonochrome(bool yesno, ParseRuntimeContext *context) {
    context->monochrome = yesno;
}

/**
Discard unneeded/unwanted entity
*/
int
MgfParserLoader::mgfDiscardUnNeededEntity(int /*ac*/, const char ** /*av*/, ParseRuntimeContext * /*context*/) {
    return MGF_OK;
}

/**
Put out current color spectrum
*/
int
MgfParserLoader::mgfPutCSpec(ParseRuntimeContext *context)
{
    char wl[2][6];
    char buffer[ColorContext::NUMBER_OF_SPECTRAL_SAMPLES][24];
    const char *newAv[ColorContext::NUMBER_OF_SPECTRAL_SAMPLES + 4];

    if ( !mgfHandlerMatches(context->readerStackState.handleCallbacks[C_SPEC], HANDLE_COLOR) ) {
        Formatter::format(wl[0], 6, "%d", COLOR_MINIMUM_WAVE_LENGTH);
        Formatter::format(wl[1], 6, "%d", COLOR_MAXIMUM_WAVE_LENGTH);
        newAv[0] = context->entityNames[C_SPEC];
        newAv[1] = wl[0];
        newAv[2] = wl[1];
        const double sf = ((double)(ColorContext::NUMBER_OF_SPECTRAL_SAMPLES)) / ((double)(context->currentColor->spectralStraightSum));
        for ( int i = 0; i < ColorContext::NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            Formatter::format(buffer[i], 24, "%.4f", sf * context->currentColor->straightSamples[i]);
            newAv[i + 3] = buffer[i];
        }
        newAv[ColorContext::NUMBER_OF_SPECTRAL_SAMPLES + 3] = NULL;
        int status = MgfEntityControl::mgfHandle(C_SPEC, ColorContext::NUMBER_OF_SPECTRAL_SAMPLES + 3, newAv, context);
        if ( status != MGF_OK ) {
            return status;
        }
    }
    return MGF_OK;
}

/**
Put out current xy chromatic values
*/
int
MgfParserLoader::mgfPutCxy(ParseRuntimeContext *context) {
    static char xBuffer[24];
    static char yBuffer[24];
    static const char *cCom[4] = {
        context->entityNames[CXY],
        xBuffer,
        yBuffer
    };

    Formatter::format(xBuffer, 24, "%.4f", context->currentColor->cx);
    Formatter::format(yBuffer, 24, "%.4f", context->currentColor->cy);
    return MgfEntityControl::mgfHandle(CXY, 3, cCom, context);
}

/**
Handle spectral color
*/
int
MgfParserLoader::mgfECSpec(int /*ac*/, const char ** /*av*/, ParseRuntimeContext *context) {
    // Convert to xy chromaticity
    context->currentColor->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
    // If it's really their handler, use it
    if ( !mgfHandlerMatches(context->readerStackState.handleCallbacks[CXY], HANDLE_COLOR) ) {
        return mgfPutCxy(context);
    }
    return MGF_OK;
}

/**
Handle mixing of colors
Contorted logic works as follows:
1. the colors are already mixed in c_h_color() support function
2. if we would handle a spectral result, make sure it's not
3. if MgfColorEntitySupport::handleColorEntity() would handle a spectral result, don't bother
4. otherwise, make c_spec entity and pass it to their handler
5. if we have only xy results, handle it as c_spec() would
*/
int
MgfParserLoader::mgfECMix(int /*ac*/, const char ** /*av*/, ParseRuntimeContext *context) {
    if ( mgfHandlerMatches(context->readerStackState.handleCallbacks[C_SPEC], COLOR_SPEC_HELPER) ) {
        context->currentColor->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
    } else if ( context->currentColor->flags & CLR_DFND_WITH_SPCTR_FLAG ) {
        return mgfPutCSpec(context);
    }
    if ( !mgfHandlerMatches(context->readerStackState.handleCallbacks[CXY], HANDLE_COLOR) ) {
        return mgfPutCxy(context);
    }
    return MGF_OK;
}

/**
Handle color temperature
*/
int
MgfParserLoader::mgfColorTemperature(int /*ac*/, const char ** /*av*/, ParseRuntimeContext *context) {
    // Logic is similar to mgfECMix here.  Support handler has already
    // converted temperature to spectral color.  Put it out as such
    // if they support it, otherwise convert to xy chromaticity and
    // put it out if they handle it
    if ( !mgfHandlerMatches(context->readerStackState.handleCallbacks[C_SPEC], COLOR_SPEC_HELPER) ) {
        return mgfPutCSpec(context);
    }
    context->currentColor->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
    if ( !mgfHandlerMatches(context->readerStackState.handleCallbacks[CXY], HANDLE_COLOR) ) {
        return mgfPutCxy(context);
    }
    return MGF_OK;
}

int
MgfParserLoader::handleIncludedFile(int ac, const char **av, ParseRuntimeContext *context) {
    const char *transformArgument[ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT];
    ReaderContext readerContext = ReaderContext();
    const TransformStackContext *originTransform = context->transformContext;

    if ( ac < 2 ) {
        return MGF_ERRR_WRNG_NUM_O_ARGMN;
    }

    int rv = MgfEntityControl::mgfOpen(&readerContext, av[1], context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    if ( ac > 2 ) {
        transformArgument[0] = context->entityNames[TRANSFORM];
        for ( int i = 1; i < ac - 1; i++ ) {
            transformArgument[i] = av[i + 1];
        }
        transformArgument[ac - 1] = NULL;
        rv = MgfEntityControl::mgfHandle(TRANSFORM, ac - 1, transformArgument, context);
        if ( rv != MGF_OK ) {
            MgfEntityControl::mgfClose(context);
            return rv;
        }
    }
    do {
        while ( (rv = mgfReadNextLine(context)) > 0 ) {
            if ( rv >= ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1 ) {
                System::err.printf("%s: %d: %s\n", readerContext.fileName,
                    readerContext.lineNumber, context->errorCodeMessages[MGF_ERROR_LINE_TOO_LONG]);
                MgfEntityControl::mgfClose(context);
                return MGF_ERROR_IN_INCLUDED_FILE;
            }
            rv = mgfParseCurrentLine(context);
            if ( rv != MGF_OK ) {
                System::err.printf("%s: %d: %s:\n%s", readerContext.fileName,
                        readerContext.lineNumber, context->errorCodeMessages[rv],
                        readerContext.inputLine);
                MgfEntityControl::mgfClose(context);
                return MGF_ERROR_IN_INCLUDED_FILE;
            }
        }
        if ( ac > 2 ) {
            rv = MgfEntityControl::mgfHandle(TRANSFORM, 1, transformArgument, context);
            if ( rv != MGF_OK ) {
                MgfEntityControl::mgfClose(context);
                return rv;
            }
        }
    } while ( context->transformContext != originTransform );
    MgfEntityControl::mgfClose(context);
    return MGF_OK;
}

void
MgfParserLoader::ensureSessionHandlerRegistry(ParseRuntimeContext *context) {
    if ( context == NULL ) {
        return;
    }

    EntityDispatchContext **handlers = context->readerStackState.handlerByType;
    if ( handlers[0] != NULL ) {
        return;
    }

    handlers[((int)(DISCARD_UNNEEDED))] = new MgfEntityHandlerAdapter(DISCARD_UNNEEDED, MgfParserLoader::mgfDiscardUnNeededEntity);
    handlers[((int)(INCLUDE_FILE))] = new MgfEntityHandlerAdapter(INCLUDE_FILE, MgfParserLoader::handleIncludedFile);
    handlers[((int)(ENTITY_SPHERE))] = new MgfEntityHandlerAdapter(ENTITY_SPHERE, MgfSphereEntityExpander::handleEntity);
    handlers[((int)(ENTITY_TORUS))] = new MgfEntityHandlerAdapter(ENTITY_TORUS, MgfTorusEntityExpander::handleEntity);
    handlers[((int)(ENTITY_CYLINDER))] = new MgfEntityHandlerAdapter(ENTITY_CYLINDER, MgfCylinderEntityExpander::handleEntity);
    handlers[((int)(ENTITY_RING))] = new MgfEntityHandlerAdapter(ENTITY_RING, MgfRingEntityTessellator::handleEntity);
    handlers[((int)(ENTITY_CONE))] = new MgfEntityHandlerAdapter(ENTITY_CONE, MgfConeEntityTessellator::handleEntity);
    handlers[((int)(ENTITY_PRISM))] = new MgfEntityHandlerAdapter(ENTITY_PRISM, MgfPrismEntityTessellator::handleEntity);
    handlers[((int)(ENTITY_FACE_WITH_HOLES))] = new MgfEntityHandlerAdapter(ENTITY_FACE_WITH_HOLES, MgfFaceWithHolesEntityExpander::handleEntity);
    handlers[((int)(COLOR_SPEC_HELPER))] = new MgfEntityHandlerAdapter(COLOR_SPEC_HELPER, MgfParserLoader::mgfECSpec);
    handlers[((int)(COLOR_MIX_HELPER))] = new MgfEntityHandlerAdapter(COLOR_MIX_HELPER, MgfParserLoader::mgfECMix);
    handlers[((int)(COLOR_TEMPERATURE_HELPER))] = new MgfEntityHandlerAdapter(COLOR_TEMPERATURE_HELPER, MgfParserLoader::mgfColorTemperature);
    handlers[((int)(HANDLE_VERTEX))] = new MgfEntityHandlerAdapter(HANDLE_VERTEX, MgfVertexFaceEntitySupport::handleVertexEntity);
    handlers[((int)(HANDLE_FACE))] = new MgfEntityHandlerAdapter(HANDLE_FACE, MgfVertexFaceEntitySupport::handleFaceEntity);
    handlers[((int)(HANDLE_FACE_WITH_HOLES))] = new MgfEntityHandlerAdapter(HANDLE_FACE_WITH_HOLES, MgfVertexFaceEntitySupport::handleFaceWithHolesEntity);
    handlers[((int)(HANDLE_SURFACE))] = new MgfEntityHandlerAdapter(HANDLE_SURFACE, MgfVertexFaceEntitySupport::handleSurfaceEntity);
    handlers[((int)(HANDLE_COLOR))] = new MgfEntityHandlerAdapter(HANDLE_COLOR, MgfColorEntitySupport::handleColorEntity);
    handlers[((int)(HANDLE_MATERIAL))] = new MgfEntityHandlerAdapter(HANDLE_MATERIAL, MgfMaterialEntitySupport::handleMaterialEntity);
    handlers[((int)(HANDLE_TRANSFORM))] = new MgfEntityHandlerAdapter(HANDLE_TRANSFORM, MgfTransformationSupport::handleTransformationEntity);
    handlers[((int)(HANDLE_OBJECT))] = new MgfEntityHandlerAdapter(HANDLE_OBJECT, MgfObjectNameSupport::handleObjectEntity);
}

EntityDispatchContext *
MgfParserLoader::mgfHandlerFromType(ParseRuntimeContext *context, HandlerRoleContext handlerType) {
    ensureSessionHandlerRegistry(context);

    const int handlerIndex = ((int)(handlerType));
    if ( handlerIndex < 0 || handlerIndex >= ReaderDispatchContext::handlerTypeCount() ) {
        Logger::fatal(-1, "mgfHandlerFromType", "Unknown MGF handler type %d", handlerIndex);
    }

    EntityDispatchContext *handler = context->readerStackState.handlerByType[handlerIndex];
    if ( handler == NULL ) {
        Logger::fatal(-1, "mgfHandlerFromType", "Missing MGF handler for type %d", handlerIndex);
    }
    return handler;
}

bool
MgfParserLoader::mgfHandlerMatches(const EntityDispatchContext *handler, HandlerRoleContext handlerType) {
    return handler != NULL && handler->type() == handlerType;
}

/**
rayCasterInitialize alternate entity handlers
*/
void
MgfParserLoader::mgfAlternativeInit(
        EntityDispatchContext *handleCallbacks[TOTAL_NUMBER_OF_ENTITIES],
        ParseRuntimeContext *context)
{
    unsigned long iNeed = 0;
    unsigned long uNeed = 0;
    int i;

    // Pick up slack
    if ( handleCallbacks[IES] == NULL) {
        handleCallbacks[IES] = mgfHandlerFromType(context, DISCARD_UNNEEDED);
    }
    if ( handleCallbacks[INCLUDE] == NULL ) {
        handleCallbacks[INCLUDE] = mgfHandlerFromType(context, INCLUDE_FILE);
    }
    if ( handleCallbacks[SPHERE] == NULL) {
        handleCallbacks[SPHERE] = mgfHandlerFromType(context, ENTITY_SPHERE);
        iNeed |= 1L << MGF_POINT | 1L << VERTEX;
    } else {
        uNeed |= 1L << MGF_POINT | 1L << VERTEX | 1L << TRANSFORM;
    }
    if ( handleCallbacks[CYLINDER] == NULL) {
        handleCallbacks[CYLINDER] = mgfHandlerFromType(context, ENTITY_CYLINDER);
        iNeed |= 1L << MGF_POINT | 1L << VERTEX;
    } else {
        uNeed |= 1L << MGF_POINT | 1L << VERTEX | 1L << TRANSFORM;
    }
    if ( handleCallbacks[CONE] == NULL) {
        handleCallbacks[CONE] = mgfHandlerFromType(context, ENTITY_CONE);
        iNeed |= 1L << MGF_POINT | 1L << VERTEX;
    } else {
        uNeed |= 1L << MGF_POINT | 1L << VERTEX | 1L << TRANSFORM;
    }
    if ( handleCallbacks[RING] == NULL) {
        handleCallbacks[RING] = mgfHandlerFromType(context, ENTITY_RING);
        iNeed |= 1L << MGF_POINT | 1L << MGF_NORMAL | 1L << VERTEX;
    } else {
        uNeed |= 1L << MGF_POINT | 1L << MGF_NORMAL | 1L << VERTEX | 1L << TRANSFORM;
    }
    if ( handleCallbacks[PRISM] == NULL) {
        handleCallbacks[PRISM] = mgfHandlerFromType(context, ENTITY_PRISM);
        iNeed |= 1L << MGF_POINT | 1L << VERTEX;
    } else {
        uNeed |= 1L << MGF_POINT | 1L << VERTEX | 1L << TRANSFORM;
    }
    if ( handleCallbacks[TORUS] == NULL) {
        handleCallbacks[TORUS] = mgfHandlerFromType(context, ENTITY_TORUS);
        iNeed |= 1L << MGF_POINT | 1L << MGF_NORMAL | 1L << VERTEX;
    } else {
        uNeed |= 1L << MGF_POINT | 1L << MGF_NORMAL | 1L << VERTEX | 1L << TRANSFORM;
    }
    if ( handleCallbacks[FACE] == NULL) {
        handleCallbacks[FACE] = handleCallbacks[FACE_WITH_HOLES];
    } else if ( handleCallbacks[FACE_WITH_HOLES] == NULL) {
        handleCallbacks[FACE_WITH_HOLES] = mgfHandlerFromType(context, ENTITY_FACE_WITH_HOLES);
    }
    if ( handleCallbacks[COLOR] != NULL) {
        if ( handleCallbacks[C_MIX] == NULL) {
            handleCallbacks[C_MIX] = mgfHandlerFromType(context, COLOR_MIX_HELPER);
            iNeed |= 1L << COLOR | 1L << CXY | 1L << C_SPEC | 1L << C_MIX | 1L << CCT;
        }
        if ( handleCallbacks[C_SPEC] == NULL) {
            handleCallbacks[C_SPEC] = mgfHandlerFromType(context, COLOR_SPEC_HELPER);
            iNeed |= 1L << COLOR | 1L << CXY | 1L << C_SPEC | 1L << C_MIX | 1L << CCT;
        }
        if ( handleCallbacks[CCT] == NULL) {
            handleCallbacks[CCT] = mgfHandlerFromType(context, COLOR_TEMPERATURE_HELPER);
            iNeed |= 1L << COLOR | 1L << CXY | 1L << C_SPEC | 1L << C_MIX | 1L << CCT;
        }
    }

    // Check for consistency
    if ( handleCallbacks[FACE] != NULL) {
        uNeed |= 1L << MGF_POINT | 1L << VERTEX | 1L << TRANSFORM;
    }
    if ( handleCallbacks[CXY] != NULL || handleCallbacks[C_SPEC] != NULL ||
         handleCallbacks[C_MIX] != NULL) {
        uNeed |= 1L << COLOR;
    }
    if ( handleCallbacks[RD] != NULL || handleCallbacks[TD] != NULL ||
         handleCallbacks[IR] != NULL ||
         handleCallbacks[ED] != NULL ||
         handleCallbacks[RS] != NULL ||
         handleCallbacks[TS] != NULL ||
         handleCallbacks[SIDES] != NULL) {
        uNeed |= 1L << MGF_MATERIAL;
    }
    for ( i = 0; i < TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( uNeed & 1L << i && handleCallbacks[i] == NULL) {
            System::err.printf("Missing support for \"%s\" entity\n",
                context->entityNames[i]);
            System::exit(1);
        }
    }

    // Add support as needed
    if ( iNeed & 1L << VERTEX && !mgfHandlerMatches(handleCallbacks[VERTEX], HANDLE_VERTEX) ) {
        context->readerStackState.supportCallbacks[VERTEX] = mgfHandlerFromType(context, HANDLE_VERTEX);
    }
    if ( iNeed & 1L << MGF_POINT && !mgfHandlerMatches(handleCallbacks[MGF_POINT], HANDLE_VERTEX) ) {
        context->readerStackState.supportCallbacks[MGF_POINT] = mgfHandlerFromType(context, HANDLE_VERTEX);
    }
    if ( iNeed & 1L << MGF_NORMAL && !mgfHandlerMatches(handleCallbacks[MGF_NORMAL], HANDLE_VERTEX) ) {
        context->readerStackState.supportCallbacks[MGF_NORMAL] = mgfHandlerFromType(context, HANDLE_VERTEX);
    }
    if ( iNeed & 1L << COLOR && !mgfHandlerMatches(handleCallbacks[COLOR], HANDLE_COLOR) ) {
        context->readerStackState.supportCallbacks[COLOR] = mgfHandlerFromType(context, HANDLE_COLOR);
    }
    if ( iNeed & 1L << CXY && !mgfHandlerMatches(handleCallbacks[CXY], HANDLE_COLOR) ) {
        context->readerStackState.supportCallbacks[CXY] = mgfHandlerFromType(context, HANDLE_COLOR);
    }
    if ( iNeed & 1L << C_SPEC && !mgfHandlerMatches(handleCallbacks[C_SPEC], HANDLE_COLOR) ) {
        context->readerStackState.supportCallbacks[C_SPEC] = mgfHandlerFromType(context, HANDLE_COLOR);
    }
    if ( iNeed & 1L << C_MIX && !mgfHandlerMatches(handleCallbacks[C_MIX], HANDLE_COLOR) ) {
        context->readerStackState.supportCallbacks[C_MIX] = mgfHandlerFromType(context, HANDLE_COLOR);
    }
    if ( iNeed & 1L << CCT && !mgfHandlerMatches(handleCallbacks[CCT], HANDLE_COLOR) ) {
        context->readerStackState.supportCallbacks[CCT] = mgfHandlerFromType(context, HANDLE_COLOR);
    }

    // Discard remaining entities
    for ( i = 0; i < TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( handleCallbacks[i] == NULL) {
            handleCallbacks[i] = mgfHandlerFromType(context, DISCARD_UNNEEDED);
        }
    }
}

void
MgfParserLoader::initMgf(ParseRuntimeContext *context) {
    // Related to ColorContext
    context->readerStackState.handleCallbacks[COLOR] = mgfHandlerFromType(context, HANDLE_COLOR);
    context->readerStackState.handleCallbacks[CXY] = mgfHandlerFromType(context, HANDLE_COLOR);
    context->readerStackState.handleCallbacks[C_MIX] = mgfHandlerFromType(context, HANDLE_COLOR);

    // Related to MgfMaterialContext
    context->readerStackState.handleCallbacks[MGF_MATERIAL] = mgfHandlerFromType(context, HANDLE_MATERIAL);
    context->readerStackState.handleCallbacks[ED] = mgfHandlerFromType(context, HANDLE_MATERIAL);
    context->readerStackState.handleCallbacks[IR] = mgfHandlerFromType(context, HANDLE_MATERIAL);
    context->readerStackState.handleCallbacks[RD] = mgfHandlerFromType(context, HANDLE_MATERIAL);
    context->readerStackState.handleCallbacks[RS] = mgfHandlerFromType(context, HANDLE_MATERIAL);
    context->readerStackState.handleCallbacks[SIDES] = mgfHandlerFromType(context, HANDLE_MATERIAL);
    context->readerStackState.handleCallbacks[TD] = mgfHandlerFromType(context, HANDLE_MATERIAL);
    context->readerStackState.handleCallbacks[TS] = mgfHandlerFromType(context, HANDLE_MATERIAL);

    // Related to TransformStackContext
    context->readerStackState.handleCallbacks[TRANSFORM] = mgfHandlerFromType(context, HANDLE_TRANSFORM);

    // Related to object, no explicit context
    context->readerStackState.handleCallbacks[OBJECT] = mgfHandlerFromType(context, HANDLE_OBJECT);

    // Related to geometry elements, no explicit context
    context->readerStackState.handleCallbacks[FACE] = mgfHandlerFromType(context, HANDLE_FACE);
    context->readerStackState.handleCallbacks[FACE_WITH_HOLES] = mgfHandlerFromType(context, HANDLE_FACE_WITH_HOLES);
    context->readerStackState.handleCallbacks[VERTEX] = mgfHandlerFromType(context, HANDLE_VERTEX);
    context->readerStackState.handleCallbacks[MGF_POINT] = mgfHandlerFromType(context, HANDLE_VERTEX);
    context->readerStackState.handleCallbacks[MGF_NORMAL] = mgfHandlerFromType(context, HANDLE_VERTEX);
    context->readerStackState.handleCallbacks[SPHERE] = mgfHandlerFromType(context, HANDLE_SURFACE);
    context->readerStackState.handleCallbacks[TORUS] = mgfHandlerFromType(context, HANDLE_SURFACE);
    context->readerStackState.handleCallbacks[RING] = mgfHandlerFromType(context, HANDLE_SURFACE);
    context->readerStackState.handleCallbacks[CYLINDER] = mgfHandlerFromType(context, HANDLE_SURFACE);
    context->readerStackState.handleCallbacks[CONE] = mgfHandlerFromType(context, HANDLE_SURFACE);
    context->readerStackState.handleCallbacks[PRISM] = mgfHandlerFromType(context, HANDLE_SURFACE);

    mgfAlternativeInit(context->readerStackState.handleCallbacks, context);
}

ParseSnapshotContext *
MgfParserLoader::mgfBuildModel(ParseRuntimeContext *context) {
    if ( context == NULL ) {
        return NULL;
    }

    if ( context->model == NULL ) {
        context->model = new ParseSnapshotContext();
    }

    ParseSnapshotContext *model = context->model;
    model->currentColor = context->currentColor;
    model->currentFaceList = context->currentFaceList;
    model->currentGeometryList = context->currentGeometryList;
    model->currentMaterialName = context->currentMaterialName;
    model->currentNormalList = context->currentNormalList;
    model->currentObjectName = context->currentObjectName;
    model->currentPointList = context->currentPointList;
    model->currentVertexList = context->currentVertexList;
    model->currentVertexName = context->currentVertexName;
    model->geometries = context->geometries;
    model->geometryStackHeadIndex = context->geometryStackHeadIndex;
    model->inComplex = context->inComplex;
    model->inSurface = context->inSurface;
    model->materials = context->materials;
    model->monochrome = context->monochrome;
    model->readerContext = context->readerContext;
    model->transformContext = context->transformContext;

    return model;
}

/**
Reads in a mgf file. The result is that the attributes
context->geometries and context->materials are filled in, and a ParseSnapshotContext
snapshot with parser outputs/state pointers is returned.

Note: this is an implementation of MGF file format with major version number 2.
*/
ParseSnapshotContext *
MgfParserLoader::readMgf(const char *filename, ParseRuntimeContext *context) {
    mgfSetNrQuartCircDivs(context->numberOfQuarterCircleDivisions);
    mgfSetMonochrome(context->monochrome, context);

    initMgf(context);

    context->currentGeometryList = new ArrayList<Geometry *>();

    if ( context->materials == NULL ) {
        context->materials = new ArrayList<Material *>();
    }

    context->geometryStackHeadIndex = 0;

    context->inComplex = false;
    context->inSurface = false;

    MgfObjectNameSupport::mgfObjectNewSurface(context);

    ReaderContext mgfReaderContext = ReaderContext();
    int status;
    if ( filename[0] == '#' ) {
        status = MgfEntityControl::mgfOpen(&mgfReaderContext, NULL, context);
    } else {
        status = MgfEntityControl::mgfOpen(&mgfReaderContext, filename, context);
    }
    if ( status ) {
        MgfEntityControl::doError(context->errorCodeMessages[status], context);
    } else {
        while ( mgfReadNextLine(context) > 0 && !status ) {
            status = mgfParseCurrentLine(context);
            if ( status ) {
                MgfEntityControl::doError(context->errorCodeMessages[status], context);
            }
        }
        MgfEntityControl::mgfClose(context);
    }
    mgfClear(context);

    if ( context->inSurface ) {
        MgfObjectNameSupport::mgfObjectSurfaceDone(context);
    }
    context->geometries = context->currentGeometryList;

    return mgfBuildModel(context);
}

void
MgfParserLoader::mgfFreeMemory(ParseRuntimeContext *context) {
    if ( context->currentGeometryList != NULL ) {
        System::out.printf("Freeing %ld geometries\n", context->currentGeometryList->size());
        long surfaces = 0;
        long patchSets = 0;
        long compounds = 0;
        long compoundChildren = 0;
        long innerCompoundChildren = 0;
        long unknowns = 0;
        for ( int i = 0; i < context->currentGeometryList->size(); i++ ) {
            const Geometry *geometry = context->currentGeometryList->get(i);
            if ( geometry->className == SURFACE_MESH ) {
                surfaces++;
            } else if ( geometry->className == PATCH_SET ) {
                patchSets++;
            } else if ( geometry->className == COMPOUND ) {
                const Compound *compound = ((const Compound *)(geometry));
                if ( compound->children != NULL ) {
                    compoundChildren += compound->children->size();
                }
                compounds++;
            } else {
                unknowns++;
            }
        }
        System::out.printf("  - MeshSurfaces: %ld\n", surfaces);
        System::out.printf("  - Patch sets: %ld\n", patchSets);
        System::out.printf("  - Compounds: %ld\n", compounds);
        System::out.printf("    . Children: %ld\n", compoundChildren);
        System::out.printf("    . Inner children: %ld\n", innerCompoundChildren);
        System::out.printf("  - Unknowns: %ld\n", unknowns);
        System::out.flush();
    }

    if ( context->allGeometries != NULL ) {
        for ( int i = 0; i < context->allGeometries->size(); i++ ) {
            delete context->allGeometries->get(i);
        }
        context->allGeometries->dispose();
    }

    if ( context->currentGeometryList != NULL ) {
        context->currentGeometryList->dispose();
        delete context->currentGeometryList;
        context->currentGeometryList = NULL;
        context->geometries = NULL;
    }

    if ( context->materials != NULL ) {
        for ( int i = 0; i < context->materials->size(); i++ ) {
            delete context->materials->get(i);
        }
        context->materials->dispose();
        delete context->materials;
        context->materials = NULL;
    }

    if ( context->currentObjectName != NULL ) {
        delete[] context->currentObjectName;
        context->currentObjectName = NULL;
    }

    if ( context->model != NULL ) {
        delete context->model;
        context->model = NULL;
    }

    MgfObjectNameSupport::mgfObjectFreeMemory(context);
    MgfTransformationSupport::mgfTransformFreeMemory(context);
    MgfEntityControl::mgfLookUpFreeMemory(context);
}
