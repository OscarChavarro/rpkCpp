#include <cstring>

#include "vsdk/toolkit/java/io/BufferedInputStream.h"
#include "vsdk/toolkit/java/lang/StringBuilder.h"
#include "vsdk/toolkit/java/lang/System.h"
#include "vsdk/toolkit/java/util/ArrayList.txx"
#include "vsdk/toolkit/java/util/Formatter.h"
#include "vsdk/toolkit/java/util/StringTokenizer.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/io/mgf/MgfEntityControl.h"
#include "vsdk/toolkit/io/mgf/MgfConeEntityTessellator.h"
#include "vsdk/toolkit/io/mgf/MgfCylinderEntityExpander.h"
#include "vsdk/toolkit/io/mgf/MgfFaceWithHolesEntityExpander.h"
#include "vsdk/toolkit/io/mgf/MgfColorEntitySupport.h"
#include "vsdk/toolkit/io/mgf/MgfVertexFaceEntitySupport.h"
#include "vsdk/toolkit/io/mgf/MgfMaterialEntitySupport.h"
#include "vsdk/toolkit/io/mgf/MgfObjectNameSupport.h"
#include "vsdk/toolkit/io/mgf/MgfPrismEntityTessellator.h"
#include "vsdk/toolkit/io/mgf/MgfRingEntityTessellator.h"
#include "vsdk/toolkit/io/mgf/MgfSphereEntityExpander.h"
#include "vsdk/toolkit/io/mgf/MgfTransformationSupport.h"
#include "vsdk/toolkit/io/mgf/MgfEntityHandlerAdapter.h"
#include "vsdk/toolkit/io/mgf/MgfTorusEntityExpander.h"
#include "vsdk/toolkit/io/mgf/MgfParserLoader.h"

/**
The parser follows the following process:
1. Fills in the handleCallbacks array with handlers for each entity
   it knows how to handle.
2. Call mgfInit to fill in the rest.  This function will report
   an error and quit if it tries to support an inconsistent set of entities.
3. For each file to parse, call mgfLoad with the file name. To read from
   standard input, use nullptr as the file name.

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
MgfParserLoader::readInputLine(java::InputStream *inputStream, char *readBuffer, int maxLength) {
    if ( inputStream == nullptr || readBuffer == nullptr || maxLength <= 0 ) {
        return 0;
    }
    int length = 0;
    while ( length < maxLength - 1 ) {
        const int readChar = inputStream->read();
        if ( readChar < 0 ) {
            break;
        }
        readBuffer[length++] = static_cast<char>(readChar);
        if ( readChar == '\n' ) {
            break;
        }
    }
    readBuffer[length] = '\0';
    return length;
}

int
MgfParserLoader::mgfReadNextLine(const ParseRuntimeContext *context) {
    if ( context->readerContext->inputStream == nullptr ) {
        return 0;
    }

    int len = 0;
    java::StringBuilder lineBuilder;
    char readBuffer[ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH];

    do {
        const int maxLength = ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - len;
        if ( maxLength <= 0 ) {
            lineBuilder.dispose();
            return len;
        }
        const int readLength = readInputLine(context->readerContext->inputStream, readBuffer, maxLength);
        if ( readLength <= 0 ) {
            java::String line = lineBuilder.toString();
            strncpy(context->readerContext->inputLine, line.toCString(), ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1);
            context->readerContext->inputLine[ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1] = '\0';
            line.dispose();
            lineBuilder.dispose();
            return len;
        }

        lineBuilder.append(readBuffer, readLength);
        len = lineBuilder.length();
        if ( len >= ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1 ) {
            java::String line = lineBuilder.toString();
            strncpy(context->readerContext->inputLine, line.toCString(), ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1);
            context->readerContext->inputLine[ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1] = '\0';
            line.dispose();
            lineBuilder.dispose();
            return len;
        }
        context->readerContext->lineNumber++;
    } while ( len > 1 && lineBuilder.charAt(len - 2) == '\\' );

    java::String line = lineBuilder.toString();
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
    java::String tokens[ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT];
    int argc = 0;

    // Copy line, removing escape chars
    java::StringBuilder buffer;
    java::String inputLine(context->readerContext->inputLine);
    for ( int i = 0; i < inputLine.length(); i++ ) {
        const char current = inputLine.charAt(i);
        const char next = inputLine.charAt(i + 1);
        if ( current == '\\' && next == '\n' ) {
            continue;
        }
        buffer.append(current);
    }

    java::StringTokenizer tokenizer(buffer.toString(), " \t\r\n\f\v");
    while ( tokenizer.hasMoreTokens() ) {
        if ( argc >= ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT - 1 ) {
            for ( int i = 0; i < argc; i++ ) {
                tokens[i].dispose();
            }
            tokenizer.dispose();
            buffer.dispose();
            inputLine.dispose();
            return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
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
        return ParseErrorContext::MGF_OK;
    }
    argv[argc] = nullptr;
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
    while ( context->readerContext != nullptr) {
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
        Logger::error(nullptr, "Number of quarter circle divisions (%d) should be positive", divs);
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
    return ParseErrorContext::MGF_OK;
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

    if ( !mgfHandlerMatches(context->readerStackState.handleCallbacks[EntityTypeContext::C_SPEC], HandlerRoleContext::HANDLE_COLOR) ) {
        java::Formatter::format(wl[0], 6, "%d", COLOR_MINIMUM_WAVE_LENGTH);
        java::Formatter::format(wl[1], 6, "%d", COLOR_MAXIMUM_WAVE_LENGTH);
        newAv[0] = context->entityNames[EntityTypeContext::C_SPEC];
        newAv[1] = wl[0];
        newAv[2] = wl[1];
        const double sf = static_cast<double>(ColorContext::NUMBER_OF_SPECTRAL_SAMPLES) / static_cast<double>(context->currentColor->spectralStraightSum);
        for ( int i = 0; i < ColorContext::NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            java::Formatter::format(buffer[i], 24, "%.4F", sf * context->currentColor->straightSamples[i]);
            newAv[i + 3] = buffer[i];
        }
        newAv[ColorContext::NUMBER_OF_SPECTRAL_SAMPLES + 3] = nullptr;
        int status = MgfEntityControl::mgfHandle(EntityTypeContext::C_SPEC, ColorContext::NUMBER_OF_SPECTRAL_SAMPLES + 3, newAv, context);
        if ( status != ParseErrorContext::MGF_OK ) {
            return status;
        }
    }
    return ParseErrorContext::MGF_OK;
}

/**
Put out current xy chromatic values
*/
int
MgfParserLoader::mgfPutCxy(ParseRuntimeContext *context) {
    static char xBuffer[24];
    static char yBuffer[24];
    static const char *cCom[4] = {
        context->entityNames[EntityTypeContext::CXY],
        xBuffer,
        yBuffer
    };

    java::Formatter::format(xBuffer, 24, "%.4F", context->currentColor->cx);
    java::Formatter::format(yBuffer, 24, "%.4F", context->currentColor->cy);
    return MgfEntityControl::mgfHandle(EntityTypeContext::CXY, 3, cCom, context);
}

/**
Handle spectral color
*/
int
MgfParserLoader::mgfECSpec(int /*ac*/, const char ** /*av*/, ParseRuntimeContext *context) {
    // Convert to xy chromaticity
    context->currentColor->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
    // If it's really their handler, use it
    if ( !mgfHandlerMatches(context->readerStackState.handleCallbacks[EntityTypeContext::CXY], HandlerRoleContext::HANDLE_COLOR) ) {
        return mgfPutCxy(context);
    }
    return ParseErrorContext::MGF_OK;
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
    if ( mgfHandlerMatches(context->readerStackState.handleCallbacks[EntityTypeContext::C_SPEC], HandlerRoleContext::COLOR_SPEC_HELPER) ) {
        context->currentColor->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
    } else if ( context->currentColor->flags & COLOR_DEFINED_WITH_SPECTRUM_FLAG ) {
        return mgfPutCSpec(context);
    }
    if ( !mgfHandlerMatches(context->readerStackState.handleCallbacks[EntityTypeContext::CXY], HandlerRoleContext::HANDLE_COLOR) ) {
        return mgfPutCxy(context);
    }
    return ParseErrorContext::MGF_OK;
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
    if ( !mgfHandlerMatches(context->readerStackState.handleCallbacks[EntityTypeContext::C_SPEC], HandlerRoleContext::COLOR_SPEC_HELPER) ) {
        return mgfPutCSpec(context);
    }
    context->currentColor->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
    if ( !mgfHandlerMatches(context->readerStackState.handleCallbacks[EntityTypeContext::CXY], HandlerRoleContext::HANDLE_COLOR) ) {
        return mgfPutCxy(context);
    }
    return ParseErrorContext::MGF_OK;
}

int
MgfParserLoader::handleIncludedFile(int ac, const char **av, ParseRuntimeContext *context) {
    const char *transformArgument[ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT];
    ReaderContext readerContext{};
    const TransformStackContext *originTransform = context->transformContext;

    if ( ac < 2 ) {
        return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }

    int rv = MgfEntityControl::mgfOpen(&readerContext, av[1], context);
    if ( rv != ParseErrorContext::MGF_OK ) {
        return rv;
    }
    if ( ac > 2 ) {
        transformArgument[0] = context->entityNames[EntityTypeContext::TRANSFORM];
        for ( int i = 1; i < ac - 1; i++ ) {
            transformArgument[i] = av[i + 1];
        }
        transformArgument[ac - 1] = nullptr;
        rv = MgfEntityControl::mgfHandle(EntityTypeContext::TRANSFORM, ac - 1, transformArgument, context);
        if ( rv != ParseErrorContext::MGF_OK ) {
            MgfEntityControl::mgfClose(context);
            return rv;
        }
    }
    do {
        while ( (rv = mgfReadNextLine(context)) > 0 ) {
            if ( rv >= ReaderContext::MGF_MAXIMUM_INPUT_LINE_LENGTH - 1 ) {
                java::System::err.printf("%s: %d: %s\n", readerContext.fileName,
                    readerContext.lineNumber, context->errorCodeMessages[ParseErrorContext::MGF_ERROR_LINE_TOO_LONG]);
                MgfEntityControl::mgfClose(context);
                return ParseErrorContext::MGF_ERROR_IN_INCLUDED_FILE;
            }
            rv = mgfParseCurrentLine(context);
            if ( rv != ParseErrorContext::MGF_OK ) {
                java::System::err.printf("%s: %d: %s:\n%s", readerContext.fileName,
                        readerContext.lineNumber, context->errorCodeMessages[rv],
                        readerContext.inputLine);
                MgfEntityControl::mgfClose(context);
                return ParseErrorContext::MGF_ERROR_IN_INCLUDED_FILE;
            }
        }
        if ( ac > 2 ) {
            rv = MgfEntityControl::mgfHandle(EntityTypeContext::TRANSFORM, 1, transformArgument, context);
            if ( rv != ParseErrorContext::MGF_OK ) {
                MgfEntityControl::mgfClose(context);
                return rv;
            }
        }
    } while ( context->transformContext != originTransform );
    MgfEntityControl::mgfClose(context);
    return ParseErrorContext::MGF_OK;
}

void
MgfParserLoader::ensureSessionHandlerRegistry(ParseRuntimeContext *context) {
    if ( context == nullptr ) {
        return;
    }

    EntityDispatchContext **handlers = context->readerStackState.handlerByType;
    if ( handlers[0] != nullptr ) {
        return;
    }

    handlers[static_cast<int>(HandlerRoleContext::DISCARD_UNNEEDED)] = new MgfEntityHandlerAdapter(HandlerRoleContext::DISCARD_UNNEEDED, MgfParserLoader::mgfDiscardUnNeededEntity);
    handlers[static_cast<int>(HandlerRoleContext::INCLUDE_FILE)] = new MgfEntityHandlerAdapter(HandlerRoleContext::INCLUDE_FILE, MgfParserLoader::handleIncludedFile);
    handlers[static_cast<int>(HandlerRoleContext::ENTITY_SPHERE)] = new MgfEntityHandlerAdapter(HandlerRoleContext::ENTITY_SPHERE, MgfSphereEntityExpander::handleEntity);
    handlers[static_cast<int>(HandlerRoleContext::ENTITY_TORUS)] = new MgfEntityHandlerAdapter(HandlerRoleContext::ENTITY_TORUS, MgfTorusEntityExpander::handleEntity);
    handlers[static_cast<int>(HandlerRoleContext::ENTITY_CYLINDER)] = new MgfEntityHandlerAdapter(HandlerRoleContext::ENTITY_CYLINDER, MgfCylinderEntityExpander::handleEntity);
    handlers[static_cast<int>(HandlerRoleContext::ENTITY_RING)] = new MgfEntityHandlerAdapter(HandlerRoleContext::ENTITY_RING, MgfRingEntityTessellator::handleEntity);
    handlers[static_cast<int>(HandlerRoleContext::ENTITY_CONE)] = new MgfEntityHandlerAdapter(HandlerRoleContext::ENTITY_CONE, MgfConeEntityTessellator::handleEntity);
    handlers[static_cast<int>(HandlerRoleContext::ENTITY_PRISM)] = new MgfEntityHandlerAdapter(HandlerRoleContext::ENTITY_PRISM, MgfPrismEntityTessellator::handleEntity);
    handlers[static_cast<int>(HandlerRoleContext::ENTITY_FACE_WITH_HOLES)] = new MgfEntityHandlerAdapter(HandlerRoleContext::ENTITY_FACE_WITH_HOLES, MgfFaceWithHolesEntityExpander::handleEntity);
    handlers[static_cast<int>(HandlerRoleContext::COLOR_SPEC_HELPER)] = new MgfEntityHandlerAdapter(HandlerRoleContext::COLOR_SPEC_HELPER, MgfParserLoader::mgfECSpec);
    handlers[static_cast<int>(HandlerRoleContext::COLOR_MIX_HELPER)] = new MgfEntityHandlerAdapter(HandlerRoleContext::COLOR_MIX_HELPER, MgfParserLoader::mgfECMix);
    handlers[static_cast<int>(HandlerRoleContext::COLOR_TEMPERATURE_HELPER)] = new MgfEntityHandlerAdapter(HandlerRoleContext::COLOR_TEMPERATURE_HELPER, MgfParserLoader::mgfColorTemperature);
    handlers[static_cast<int>(HandlerRoleContext::HANDLE_VERTEX)] = new MgfEntityHandlerAdapter(HandlerRoleContext::HANDLE_VERTEX, MgfVertexFaceEntitySupport::handleVertexEntity);
    handlers[static_cast<int>(HandlerRoleContext::HANDLE_FACE)] = new MgfEntityHandlerAdapter(HandlerRoleContext::HANDLE_FACE, MgfVertexFaceEntitySupport::handleFaceEntity);
    handlers[static_cast<int>(HandlerRoleContext::HANDLE_FACE_WITH_HOLES)] = new MgfEntityHandlerAdapter(HandlerRoleContext::HANDLE_FACE_WITH_HOLES, MgfVertexFaceEntitySupport::handleFaceWithHolesEntity);
    handlers[static_cast<int>(HandlerRoleContext::HANDLE_SURFACE)] = new MgfEntityHandlerAdapter(HandlerRoleContext::HANDLE_SURFACE, MgfVertexFaceEntitySupport::handleSurfaceEntity);
    handlers[static_cast<int>(HandlerRoleContext::HANDLE_COLOR)] = new MgfEntityHandlerAdapter(HandlerRoleContext::HANDLE_COLOR, MgfColorEntitySupport::handleColorEntity);
    handlers[static_cast<int>(HandlerRoleContext::HANDLE_MATERIAL)] = new MgfEntityHandlerAdapter(HandlerRoleContext::HANDLE_MATERIAL, MgfMaterialEntitySupport::handleMaterialEntity);
    handlers[static_cast<int>(HandlerRoleContext::HANDLE_TRANSFORM)] = new MgfEntityHandlerAdapter(HandlerRoleContext::HANDLE_TRANSFORM, MgfTransformationSupport::handleTransformationEntity);
    handlers[static_cast<int>(HandlerRoleContext::HANDLE_OBJECT)] = new MgfEntityHandlerAdapter(HandlerRoleContext::HANDLE_OBJECT, MgfObjectNameSupport::handleObjectEntity);
}

EntityDispatchContext *
MgfParserLoader::mgfHandlerFromType(ParseRuntimeContext *context, HandlerRoleContext handlerType) {
    ensureSessionHandlerRegistry(context);

    const int handlerIndex = static_cast<int>(handlerType);
    if ( handlerIndex < 0 || handlerIndex >= ReaderDispatchContext::handlerTypeCount() ) {
        Logger::fatal(-1, "mgfHandlerFromType", "Unknown MGF handler type %d", handlerIndex);
    }

    EntityDispatchContext *handler = context->readerStackState.handlerByType[handlerIndex];
    if ( handler == nullptr ) {
        Logger::fatal(-1, "mgfHandlerFromType", "Missing MGF handler for type %d", handlerIndex);
    }
    return handler;
}

bool
MgfParserLoader::mgfHandlerMatches(const EntityDispatchContext *handler, HandlerRoleContext handlerType) {
    return handler != nullptr && handler->type() == handlerType;
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
    if ( handleCallbacks[IES] == nullptr) {
        handleCallbacks[IES] = mgfHandlerFromType(context, HandlerRoleContext::DISCARD_UNNEEDED);
    }
    if ( handleCallbacks[INCLUDE] == nullptr ) {
        handleCallbacks[INCLUDE] = mgfHandlerFromType(context, HandlerRoleContext::INCLUDE_FILE);
    }
    if ( handleCallbacks[EntityTypeContext::SPHERE] == nullptr) {
        handleCallbacks[EntityTypeContext::SPHERE] = mgfHandlerFromType(context, HandlerRoleContext::ENTITY_SPHERE);
        iNeed |= 1L << EntityTypeContext::MGF_POINT | 1L << EntityTypeContext::VERTEX;
    } else {
        uNeed |= 1L << EntityTypeContext::MGF_POINT | 1L << EntityTypeContext::VERTEX | 1L << EntityTypeContext::TRANSFORM;
    }
    if ( handleCallbacks[EntityTypeContext::CYLINDER] == nullptr) {
        handleCallbacks[EntityTypeContext::CYLINDER] = mgfHandlerFromType(context, HandlerRoleContext::ENTITY_CYLINDER);
        iNeed |= 1L << EntityTypeContext::MGF_POINT | 1L << EntityTypeContext::VERTEX;
    } else {
        uNeed |= 1L << EntityTypeContext::MGF_POINT | 1L << EntityTypeContext::VERTEX | 1L << EntityTypeContext::TRANSFORM;
    }
    if ( handleCallbacks[EntityTypeContext::CONE] == nullptr) {
        handleCallbacks[EntityTypeContext::CONE] = mgfHandlerFromType(context, HandlerRoleContext::ENTITY_CONE);
        iNeed |= 1L << EntityTypeContext::MGF_POINT | 1L << EntityTypeContext::VERTEX;
    } else {
        uNeed |= 1L << EntityTypeContext::MGF_POINT | 1L << EntityTypeContext::VERTEX | 1L << EntityTypeContext::TRANSFORM;
    }
    if ( handleCallbacks[EntityTypeContext::RING] == nullptr) {
        handleCallbacks[EntityTypeContext::RING] = mgfHandlerFromType(context, HandlerRoleContext::ENTITY_RING);
        iNeed |= 1L << EntityTypeContext::MGF_POINT | 1L << EntityTypeContext::MGF_NORMAL | 1L << EntityTypeContext::VERTEX;
    } else {
        uNeed |= 1L << EntityTypeContext::MGF_POINT | 1L << EntityTypeContext::MGF_NORMAL | 1L << EntityTypeContext::VERTEX | 1L << EntityTypeContext::TRANSFORM;
    }
    if ( handleCallbacks[EntityTypeContext::PRISM] == nullptr) {
        handleCallbacks[EntityTypeContext::PRISM] = mgfHandlerFromType(context, HandlerRoleContext::ENTITY_PRISM);
        iNeed |= 1L << EntityTypeContext::MGF_POINT | 1L << EntityTypeContext::VERTEX;
    } else {
        uNeed |= 1L << EntityTypeContext::MGF_POINT | 1L << EntityTypeContext::VERTEX | 1L << EntityTypeContext::TRANSFORM;
    }
    if ( handleCallbacks[EntityTypeContext::TORUS] == nullptr) {
        handleCallbacks[EntityTypeContext::TORUS] = mgfHandlerFromType(context, HandlerRoleContext::ENTITY_TORUS);
        iNeed |= 1L << EntityTypeContext::MGF_POINT | 1L << EntityTypeContext::MGF_NORMAL | 1L << EntityTypeContext::VERTEX;
    } else {
        uNeed |= 1L << EntityTypeContext::MGF_POINT | 1L << EntityTypeContext::MGF_NORMAL | 1L << EntityTypeContext::VERTEX | 1L << EntityTypeContext::TRANSFORM;
    }
    if ( handleCallbacks[EntityTypeContext::FACE] == nullptr) {
        handleCallbacks[EntityTypeContext::FACE] = handleCallbacks[EntityTypeContext::FACE_WITH_HOLES];
    } else if ( handleCallbacks[EntityTypeContext::FACE_WITH_HOLES] == nullptr) {
        handleCallbacks[EntityTypeContext::FACE_WITH_HOLES] = mgfHandlerFromType(context, HandlerRoleContext::ENTITY_FACE_WITH_HOLES);
    }
    if ( handleCallbacks[EntityTypeContext::COLOR] != nullptr) {
        if ( handleCallbacks[EntityTypeContext::C_MIX] == nullptr) {
            handleCallbacks[EntityTypeContext::C_MIX] = mgfHandlerFromType(context, HandlerRoleContext::COLOR_MIX_HELPER);
            iNeed |= 1L << EntityTypeContext::COLOR | 1L << EntityTypeContext::CXY | 1L << EntityTypeContext::C_SPEC | 1L << EntityTypeContext::C_MIX | 1L << EntityTypeContext::CCT;
        }
        if ( handleCallbacks[EntityTypeContext::C_SPEC] == nullptr) {
            handleCallbacks[EntityTypeContext::C_SPEC] = mgfHandlerFromType(context, HandlerRoleContext::COLOR_SPEC_HELPER);
            iNeed |= 1L << EntityTypeContext::COLOR | 1L << EntityTypeContext::CXY | 1L << EntityTypeContext::C_SPEC | 1L << EntityTypeContext::C_MIX | 1L << EntityTypeContext::CCT;
        }
        if ( handleCallbacks[EntityTypeContext::CCT] == nullptr) {
            handleCallbacks[EntityTypeContext::CCT] = mgfHandlerFromType(context, HandlerRoleContext::COLOR_TEMPERATURE_HELPER);
            iNeed |= 1L << EntityTypeContext::COLOR | 1L << EntityTypeContext::CXY | 1L << EntityTypeContext::C_SPEC | 1L << EntityTypeContext::C_MIX | 1L << EntityTypeContext::CCT;
        }
    }

    // Check for consistency
    if ( handleCallbacks[EntityTypeContext::FACE] != nullptr) {
        uNeed |= 1L << EntityTypeContext::MGF_POINT | 1L << EntityTypeContext::VERTEX | 1L << EntityTypeContext::TRANSFORM;
    }
    if ( handleCallbacks[EntityTypeContext::CXY] != nullptr || handleCallbacks[EntityTypeContext::C_SPEC] != nullptr ||
         handleCallbacks[EntityTypeContext::C_MIX] != nullptr) {
        uNeed |= 1L << EntityTypeContext::COLOR;
    }
    if ( handleCallbacks[EntityTypeContext::RD] != nullptr || handleCallbacks[EntityTypeContext::TD] != nullptr ||
         handleCallbacks[EntityTypeContext::IR] != nullptr ||
         handleCallbacks[EntityTypeContext::ED] != nullptr ||
         handleCallbacks[EntityTypeContext::RS] != nullptr ||
         handleCallbacks[EntityTypeContext::TS] != nullptr ||
         handleCallbacks[EntityTypeContext::SIDES] != nullptr) {
        uNeed |= 1L << EntityTypeContext::MGF_MATERIAL;
    }
    for ( i = 0; i < TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( uNeed & 1L << i && handleCallbacks[i] == nullptr) {
            java::System::err.printf("Missing support for \"%s\" entity\n",
                context->entityNames[i]);
            java::System::exit(1);
        }
    }

    // Add support as needed
    if ( iNeed & 1L << EntityTypeContext::VERTEX && !mgfHandlerMatches(handleCallbacks[EntityTypeContext::VERTEX], HandlerRoleContext::HANDLE_VERTEX) ) {
        context->readerStackState.supportCallbacks[EntityTypeContext::VERTEX] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_VERTEX);
    }
    if ( iNeed & 1L << EntityTypeContext::MGF_POINT && !mgfHandlerMatches(handleCallbacks[EntityTypeContext::MGF_POINT], HandlerRoleContext::HANDLE_VERTEX) ) {
        context->readerStackState.supportCallbacks[EntityTypeContext::MGF_POINT] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_VERTEX);
    }
    if ( iNeed & 1L << EntityTypeContext::MGF_NORMAL && !mgfHandlerMatches(handleCallbacks[EntityTypeContext::MGF_NORMAL], HandlerRoleContext::HANDLE_VERTEX) ) {
        context->readerStackState.supportCallbacks[EntityTypeContext::MGF_NORMAL] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_VERTEX);
    }
    if ( iNeed & 1L << EntityTypeContext::COLOR && !mgfHandlerMatches(handleCallbacks[EntityTypeContext::COLOR], HandlerRoleContext::HANDLE_COLOR) ) {
        context->readerStackState.supportCallbacks[EntityTypeContext::COLOR] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_COLOR);
    }
    if ( iNeed & 1L << EntityTypeContext::CXY && !mgfHandlerMatches(handleCallbacks[EntityTypeContext::CXY], HandlerRoleContext::HANDLE_COLOR) ) {
        context->readerStackState.supportCallbacks[EntityTypeContext::CXY] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_COLOR);
    }
    if ( iNeed & 1L << EntityTypeContext::C_SPEC && !mgfHandlerMatches(handleCallbacks[EntityTypeContext::C_SPEC], HandlerRoleContext::HANDLE_COLOR) ) {
        context->readerStackState.supportCallbacks[EntityTypeContext::C_SPEC] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_COLOR);
    }
    if ( iNeed & 1L << EntityTypeContext::C_MIX && !mgfHandlerMatches(handleCallbacks[EntityTypeContext::C_MIX], HandlerRoleContext::HANDLE_COLOR) ) {
        context->readerStackState.supportCallbacks[EntityTypeContext::C_MIX] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_COLOR);
    }
    if ( iNeed & 1L << EntityTypeContext::CCT && !mgfHandlerMatches(handleCallbacks[EntityTypeContext::CCT], HandlerRoleContext::HANDLE_COLOR) ) {
        context->readerStackState.supportCallbacks[EntityTypeContext::CCT] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_COLOR);
    }

    // Discard remaining entities
    for ( i = 0; i < TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( handleCallbacks[i] == nullptr) {
            handleCallbacks[i] = mgfHandlerFromType(context, HandlerRoleContext::DISCARD_UNNEEDED);
        }
    }
}

void
MgfParserLoader::initMgf(ParseRuntimeContext *context) {
    // Related to ColorContext
    context->readerStackState.handleCallbacks[EntityTypeContext::COLOR] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_COLOR);
    context->readerStackState.handleCallbacks[EntityTypeContext::CXY] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_COLOR);
    context->readerStackState.handleCallbacks[EntityTypeContext::C_MIX] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_COLOR);

    // Related to MgfMaterialContext
    context->readerStackState.handleCallbacks[EntityTypeContext::MGF_MATERIAL] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_MATERIAL);
    context->readerStackState.handleCallbacks[EntityTypeContext::ED] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_MATERIAL);
    context->readerStackState.handleCallbacks[EntityTypeContext::IR] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_MATERIAL);
    context->readerStackState.handleCallbacks[EntityTypeContext::RD] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_MATERIAL);
    context->readerStackState.handleCallbacks[EntityTypeContext::RS] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_MATERIAL);
    context->readerStackState.handleCallbacks[EntityTypeContext::SIDES] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_MATERIAL);
    context->readerStackState.handleCallbacks[EntityTypeContext::TD] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_MATERIAL);
    context->readerStackState.handleCallbacks[EntityTypeContext::TS] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_MATERIAL);

    // Related to TransformStackContext
    context->readerStackState.handleCallbacks[EntityTypeContext::TRANSFORM] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_TRANSFORM);

    // Related to object, no explicit context
    context->readerStackState.handleCallbacks[EntityTypeContext::OBJECT] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_OBJECT);

    // Related to geometry elements, no explicit context
    context->readerStackState.handleCallbacks[EntityTypeContext::FACE] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_FACE);
    context->readerStackState.handleCallbacks[EntityTypeContext::FACE_WITH_HOLES] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_FACE_WITH_HOLES);
    context->readerStackState.handleCallbacks[EntityTypeContext::VERTEX] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_VERTEX);
    context->readerStackState.handleCallbacks[EntityTypeContext::MGF_POINT] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_VERTEX);
    context->readerStackState.handleCallbacks[EntityTypeContext::MGF_NORMAL] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_VERTEX);
    context->readerStackState.handleCallbacks[EntityTypeContext::SPHERE] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_SURFACE);
    context->readerStackState.handleCallbacks[EntityTypeContext::TORUS] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_SURFACE);
    context->readerStackState.handleCallbacks[EntityTypeContext::RING] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_SURFACE);
    context->readerStackState.handleCallbacks[EntityTypeContext::CYLINDER] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_SURFACE);
    context->readerStackState.handleCallbacks[EntityTypeContext::CONE] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_SURFACE);
    context->readerStackState.handleCallbacks[EntityTypeContext::PRISM] = mgfHandlerFromType(context, HandlerRoleContext::HANDLE_SURFACE);

    mgfAlternativeInit(context->readerStackState.handleCallbacks, context);
}

ParseSnapshotContext *
MgfParserLoader::mgfBuildModel(ParseRuntimeContext *context) {
    if ( context == nullptr ) {
        return nullptr;
    }

    if ( context->model == nullptr ) {
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

    context->currentGeometryList = new java::ArrayList<Geometry *>();

    if ( context->materials == nullptr ) {
        context->materials = new java::ArrayList<Material *>();
    }

    context->geometryStackHeadIndex = 0;

    context->inComplex = false;
    context->inSurface = false;

    MgfObjectNameSupport::mgfObjectNewSurface(context);

    ReaderContext mgfReaderContext{};
    int status;
    if ( filename[0] == '#' ) {
        status = MgfEntityControl::mgfOpen(&mgfReaderContext, nullptr, context);
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
    if ( context->currentGeometryList != nullptr ) {
        java::System::out.printf("Freeing %ld geometries\n", context->currentGeometryList->size());
        long surfaces = 0;
        long patchSets = 0;
        long compounds = 0;
        long compoundChildren = 0;
        long innerCompoundChildren = 0;
        long unknowns = 0;
        for ( int i = 0; i < context->currentGeometryList->size(); i++ ) {
            const Geometry *geometry = context->currentGeometryList->get(i);
            if ( geometry->className == GeometryClassId::SURFACE_MESH ) {
                surfaces++;
            } else if ( geometry->className == GeometryClassId::PATCH_SET ) {
                patchSets++;
            } else if ( geometry->className == GeometryClassId::COMPOUND ) {
                const Compound *compound = dynamic_cast<const Compound *>(geometry);
                if ( compound->children != nullptr ) {
                    compoundChildren += compound->children->size();
                }
                compounds++;
            } else {
                unknowns++;
            }
        }
        java::System::out.printf("  - MeshSurfaces: %ld\n", surfaces);
        java::System::out.printf("  - Patch sets: %ld\n", patchSets);
        java::System::out.printf("  - Compounds: %ld\n", compounds);
        java::System::out.printf("    . Children: %ld\n", compoundChildren);
        java::System::out.printf("    . Inner children: %ld\n", innerCompoundChildren);
        java::System::out.printf("  - Unknowns: %ld\n", unknowns);
        java::System::out.flush();
    }

    if ( context->allGeometries != nullptr ) {
        for ( int i = 0; i < context->allGeometries->size(); i++ ) {
            delete context->allGeometries->get(i);
        }
        context->allGeometries->dispose();
    }

    if ( context->currentGeometryList != nullptr ) {
        context->currentGeometryList->dispose();
        delete context->currentGeometryList;
        context->currentGeometryList = nullptr;
        context->geometries = nullptr;
    }

    if ( context->materials != nullptr ) {
        for ( int i = 0; i < context->materials->size(); i++ ) {
            delete context->materials->get(i);
        }
        context->materials->dispose();
        delete context->materials;
        context->materials = nullptr;
    }

    if ( context->currentObjectName != nullptr ) {
        delete[] context->currentObjectName;
        context->currentObjectName = nullptr;
    }

    if ( context->model != nullptr ) {
        delete context->model;
        context->model = nullptr;
    }

    MgfObjectNameSupport::mgfObjectFreeMemory(context);
    MgfTransformationSupport::mgfTransformFreeMemory(context);
    MgfEntityControl::mgfLookUpFreeMemory(context);
}
