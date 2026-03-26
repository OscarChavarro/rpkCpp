#include <cstring>
#include "java/lang/System.h"

#include "java/util/ArrayList.txx"
#include "java/util/StringTokenizer.h"
#include "java/lang/String.h"
#include "java/lang/StringBuilder.h"
#include "java/io/BufferedInputStream.h"
#include "common/error.h"
#include "io/mgf/MgfColorContext.h"
#include "io/mgf/MgfTransformContext.h"
#include "io/mgf/mgfHandlerGeometry.h"
#include "io/mgf/mgfHandlerTransform.h"
#include "io/mgf/mgfHandlerObject.h"
#include "io/mgf/mgfGeometry.h"
#include "io/mgf/mgfHandlerColor.h"
#include "io/mgf/mgfHandlerMaterial.h"
#include "io/mgf/readmgf.h"
#include "io/mgf/mgfDefinitions.h"

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
To globalPass an entity of your own construction to the parser, use
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
static int
mgfReadNextLine(const MgfContext *context) {
    if ( context->readerContext->inputStream == nullptr ) {
        return 0;
    }

    int len = 0;
    java::lang::StringBuilder lineBuilder;
    char readBuffer[MGF_MAXIMUM_INPUT_LINE_LENGTH];

    do {
        const int maxLength = MGF_MAXIMUM_INPUT_LINE_LENGTH - len;
        if ( maxLength <= 0 ) {
            lineBuilder.dispose();
            return len;
        }
        const int readLength = context->readerContext->inputStream->readLine(readBuffer, maxLength);
        if ( readLength <= 0 ) {
            java::lang::String line = lineBuilder.toString();
            strncpy(context->readerContext->inputLine, line.toCString(), MGF_MAXIMUM_INPUT_LINE_LENGTH - 1);
            context->readerContext->inputLine[MGF_MAXIMUM_INPUT_LINE_LENGTH - 1] = '\0';
            line.dispose();
            lineBuilder.dispose();
            return len;
        }

        lineBuilder.append(readBuffer, readLength);
        len = lineBuilder.length();
        if ( len >= MGF_MAXIMUM_INPUT_LINE_LENGTH - 1 ) {
            java::lang::String line = lineBuilder.toString();
            strncpy(context->readerContext->inputLine, line.toCString(), MGF_MAXIMUM_INPUT_LINE_LENGTH - 1);
            context->readerContext->inputLine[MGF_MAXIMUM_INPUT_LINE_LENGTH - 1] = '\0';
            line.dispose();
            lineBuilder.dispose();
            return len;
        }
        context->readerContext->lineNumber++;
    } while ( len > 1 && lineBuilder.charAt(len - 2) == '\\' );

    java::lang::String line = lineBuilder.toString();
    strncpy(context->readerContext->inputLine, line.toCString(), MGF_MAXIMUM_INPUT_LINE_LENGTH - 1);
    context->readerContext->inputLine[MGF_MAXIMUM_INPUT_LINE_LENGTH - 1] = '\0';
    line.dispose();
    lineBuilder.dispose();

    return len;
}

/**
Parse current input line
*/
static int
mgfParseCurrentLine(MgfContext *context) {
    const char *argv[MGF_MAXIMUM_ARGUMENT_COUNT];
    java::lang::String tokens[MGF_MAXIMUM_ARGUMENT_COUNT];
    int argc = 0;

    // Copy line, removing escape chars
    java::lang::StringBuilder buffer;
    java::lang::String inputLine(context->readerContext->inputLine);
    for ( int i = 0; i < inputLine.length(); i++ ) {
        const char current = inputLine.charAt(i);
        const char next = inputLine.charAt(i + 1);
        if ( current == '\\' && next == '\n' ) {
            continue;
        }
        buffer.append(current);
    }

    java::util::StringTokenizer tokenizer(buffer.toString(), " \t\r\n\f\v");
    while ( tokenizer.hasMoreTokens() ) {
        if ( argc >= MGF_MAXIMUM_ARGUMENT_COUNT - 1 ) {
            for ( int i = 0; i < argc; i++ ) {
                tokens[i].dispose();
            }
            tokenizer.dispose();
            buffer.dispose();
            inputLine.dispose();
            return MgfErrorCode::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
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
        return MgfErrorCode::MGF_OK;
    }
    argv[argc] = nullptr;
    tokenizer.dispose();
    buffer.dispose();
    inputLine.dispose();
    // Else handle it
    const int status = mgfHandle(-1, argc, argv, context);
    for ( int i = 0; i < argc; i++ ) {
        tokens[i].dispose();
    }
    return status;
}

/**
Clear parser history
*/
static void
mgfClear(MgfContext *context) {
    initColorContextTables(context);
    initGeometryContextTables(context);
    initMaterialContextTables(context);
    while ( context->readerContext != nullptr) {
        // Reset our file context
        mgfClose(context);
    }
}

/**
Sets the number of quarter circle divisions for discrete approximation of cylinders, spheres, cones, etc.
*/
static void
mgfSetNrQuartCircDivs(int divs) {
    if ( divs <= 0 ) {
        logError(nullptr, "Number of quarter circle divisions (%d) should be positive", divs);
        return;
    }
}

/**
If yesno is true, all materials will be converted to be monochrome
*/
static void
mgfSetMonochrome(bool yesno, MgfContext *context) {
    context->monochrome = yesno;
}

/**
Discard unneeded/unwanted entity
*/
static int
mgfDiscardUnNeededEntity(int /*ac*/, const char ** /*av*/, MgfContext * /*context*/) {
    return MgfErrorCode::MGF_OK;
}

/**
Put out current color spectrum
*/
static int
mgfPutCSpec(MgfContext *context)
{
    char wl[2][6];
    char buffer[NUMBER_OF_SPECTRAL_SAMPLES][24];
    const char *newAv[NUMBER_OF_SPECTRAL_SAMPLES + 4];

    if ( !mgfHandlerMatches(context->handleCallbacks[MgfEntity::C_SPEC], MgfHandlerType::HANDLE_COLOR) ) {
        snprintf(wl[0], 6, "%d", COLOR_MINIMUM_WAVE_LENGTH);
        snprintf(wl[1], 6, "%d", COLOR_MAXIMUM_WAVE_LENGTH);
        newAv[0] = context->entityNames[MgfEntity::C_SPEC];
        newAv[1] = wl[0];
        newAv[2] = wl[1];
        const double sf = static_cast<double>(NUMBER_OF_SPECTRAL_SAMPLES) / static_cast<double>(context->currentColor->spectralStraightSum);
        for ( int i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            snprintf(buffer[i], 24, "%.4f", sf * context->currentColor->straightSamples[i]);
            newAv[i + 3] = buffer[i];
        }
        newAv[NUMBER_OF_SPECTRAL_SAMPLES + 3] = nullptr;
        int status = mgfHandle(MgfEntity::C_SPEC, NUMBER_OF_SPECTRAL_SAMPLES + 3, newAv, context);
        if ( status != MgfErrorCode::MGF_OK ) {
            return status;
        }
    }
    return MgfErrorCode::MGF_OK;
}

/**
Put out current xy chromatic values
*/
static int
mgfPutCxy(MgfContext *context) {
    static char xBuffer[24];
    static char yBuffer[24];
    static const char *cCom[4] = {
        context->entityNames[MgfEntity::CXY],
        xBuffer,
        yBuffer
    };

    snprintf(xBuffer, 24, "%.4f", context->currentColor->cx);
    snprintf(yBuffer, 24, "%.4f", context->currentColor->cy);
    return mgfHandle(MgfEntity::CXY, 3, cCom, context);
}

/**
Handle spectral color
*/
static int
mgfECSpec(int /*ac*/, const char ** /*av*/, MgfContext *context) {
    // Convert to xy chromaticity
    context->currentColor->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
    // If it's really their handler, use it
    if ( !mgfHandlerMatches(context->handleCallbacks[MgfEntity::CXY], MgfHandlerType::HANDLE_COLOR) ) {
        return mgfPutCxy(context);
    }
    return MgfErrorCode::MGF_OK;
}

/**
Handle mixing of colors
Contorted logic works as follows:
1. the colors are already mixed in c_h_color() support function
2. if we would handle a spectral result, make sure it's not
3. if handleColorEntity() would handle a spectral result, don't bother
4. otherwise, make c_spec entity and pass it to their handler
5. if we have only xy results, handle it as c_spec() would
*/
static int
mgfECMix(int /*ac*/, const char ** /*av*/, MgfContext *context) {
    if ( mgfHandlerMatches(context->handleCallbacks[MgfEntity::C_SPEC], MgfHandlerType::COLOR_SPEC_HELPER) ) {
        context->currentColor->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
    } else if ( context->currentColor->flags & COLOR_DEFINED_WITH_SPECTRUM_FLAG ) {
        return mgfPutCSpec(context);
    }
    if ( !mgfHandlerMatches(context->handleCallbacks[MgfEntity::CXY], MgfHandlerType::HANDLE_COLOR) ) {
        return mgfPutCxy(context);
    }
    return MgfErrorCode::MGF_OK;
}

/**
Handle color temperature
*/
static int
mgfColorTemperature(int /*ac*/, const char ** /*av*/, MgfContext *context) {
    // Logic is similar to mgfECMix here.  Support handler has already
    // converted temperature to spectral color.  Put it out as such
    // if they support it, otherwise convert to xy chromaticity and
    // put it out if they handle it
    if ( !mgfHandlerMatches(context->handleCallbacks[MgfEntity::C_SPEC], MgfHandlerType::COLOR_SPEC_HELPER) ) {
        return mgfPutCSpec(context);
    }
    context->currentColor->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
    if ( !mgfHandlerMatches(context->handleCallbacks[MgfEntity::CXY], MgfHandlerType::HANDLE_COLOR) ) {
        return mgfPutCxy(context);
    }
    return MgfErrorCode::MGF_OK;
}

static int
handleIncludedFile(int ac, const char **av, MgfContext *context) {
    const char *transformArgument[MGF_MAXIMUM_ARGUMENT_COUNT];
    MgfReaderContext readerContext{};
    const MgfTransformContext *originTransform = context->transformContext;

    if ( ac < 2 ) {
        return MgfErrorCode::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }

    int rv = mgfOpen(&readerContext, av[1], context);
    if ( rv != MgfErrorCode::MGF_OK ) {
        return rv;
    }
    if ( ac > 2 ) {
        transformArgument[0] = context->entityNames[MgfEntity::TRANSFORM];
        for ( int i = 1; i < ac - 1; i++ ) {
            transformArgument[i] = av[i + 1];
        }
        transformArgument[ac - 1] = nullptr;
        rv = mgfHandle(MgfEntity::TRANSFORM, ac - 1, transformArgument, context);
        if ( rv != MgfErrorCode::MGF_OK ) {
            mgfClose(context);
            return rv;
        }
    }
    do {
        while ( (rv = mgfReadNextLine(context)) > 0 ) {
            if ( rv >= MGF_MAXIMUM_INPUT_LINE_LENGTH - 1 ) {
                java::lang::System::err.printf("%s: %d: %s\n", readerContext.fileName,
                    readerContext.lineNumber, context->errorCodeMessages[MgfErrorCode::MGF_ERROR_LINE_TOO_LONG]);
                mgfClose(context);
                return MgfErrorCode::MGF_ERROR_IN_INCLUDED_FILE;
            }
            rv = mgfParseCurrentLine(context);
            if ( rv != MgfErrorCode::MGF_OK ) {
                java::lang::System::err.printf("%s: %d: %s:\n%s", readerContext.fileName,
                        readerContext.lineNumber, context->errorCodeMessages[rv],
                        readerContext.inputLine);
                mgfClose(context);
                return MgfErrorCode::MGF_ERROR_IN_INCLUDED_FILE;
            }
        }
        if ( ac > 2 ) {
            rv = mgfHandle(MgfEntity::TRANSFORM, 1, transformArgument, context);
            if ( rv != MgfErrorCode::MGF_OK ) {
                mgfClose(context);
                return rv;
            }
        }
    } while ( context->transformContext != originTransform );
    mgfClose(context);
    return MgfErrorCode::MGF_OK;
}

class MgfStaticHandler final : public MgfEntityHandler {
  public:
    explicit MgfStaticHandler(const MgfHandlerType handlerType):
        handlerType(handlerType)
    {
    }

    int
    handle(int argc, const char **argv, MgfContext *context) const override {
        switch ( handlerType ) {
            case MgfHandlerType::DISCARD_UNNEEDED:
                return mgfDiscardUnNeededEntity(argc, argv, context);
            case MgfHandlerType::INCLUDE_FILE:
                return handleIncludedFile(argc, argv, context);
            case MgfHandlerType::ENTITY_SPHERE:
                return mgfEntitySphere(argc, argv, context);
            case MgfHandlerType::ENTITY_TORUS:
                return mgfEntityTorus(argc, argv, context);
            case MgfHandlerType::ENTITY_CYLINDER:
                return mgfEntityCylinder(argc, argv, context);
            case MgfHandlerType::ENTITY_RING:
                return mgfEntityRing(argc, argv, context);
            case MgfHandlerType::ENTITY_CONE:
                return mgfEntityCone(argc, argv, context);
            case MgfHandlerType::ENTITY_PRISM:
                return mgfEntityPrism(argc, argv, context);
            case MgfHandlerType::ENTITY_FACE_WITH_HOLES:
                return mgfEntityFaceWithHoles(argc, argv, context);
            case MgfHandlerType::COLOR_SPEC_HELPER:
                return mgfECSpec(argc, argv, context);
            case MgfHandlerType::COLOR_MIX_HELPER:
                return mgfECMix(argc, argv, context);
            case MgfHandlerType::COLOR_TEMPERATURE_HELPER:
                return mgfColorTemperature(argc, argv, context);
            case MgfHandlerType::HANDLE_VERTEX:
                return handleVertexEntity(argc, argv, context);
            case MgfHandlerType::HANDLE_FACE:
                return handleFaceEntity(argc, argv, context);
            case MgfHandlerType::HANDLE_FACE_WITH_HOLES:
                return handleFaceWithHolesEntity(argc, argv, context);
            case MgfHandlerType::HANDLE_SURFACE:
                return handleSurfaceEntity(argc, argv, context);
            case MgfHandlerType::HANDLE_COLOR:
                return handleColorEntity(argc, argv, context);
            case MgfHandlerType::HANDLE_MATERIAL:
                return handleMaterialEntity(argc, argv, context);
            case MgfHandlerType::HANDLE_TRANSFORM:
                return handleTransformationEntity(argc, argv, context);
            case MgfHandlerType::HANDLE_OBJECT:
                return handleObjectEntity(argc, argv, context);
            default:
                return MgfErrorCode::MGF_OK;
        }
    }

    MgfHandlerType
    type() const override {
        return handlerType;
    }

  private:
    MgfHandlerType handlerType;
};

namespace {
static constexpr int TOTAL_MGF_HANDLER_TYPES = static_cast<int>(MgfHandlerType::HANDLE_OBJECT) + 1;

static MgfStaticHandler globalMgfHandlers[TOTAL_MGF_HANDLER_TYPES] = {
    MgfStaticHandler(MgfHandlerType::DISCARD_UNNEEDED),
    MgfStaticHandler(MgfHandlerType::INCLUDE_FILE),
    MgfStaticHandler(MgfHandlerType::ENTITY_SPHERE),
    MgfStaticHandler(MgfHandlerType::ENTITY_TORUS),
    MgfStaticHandler(MgfHandlerType::ENTITY_CYLINDER),
    MgfStaticHandler(MgfHandlerType::ENTITY_RING),
    MgfStaticHandler(MgfHandlerType::ENTITY_CONE),
    MgfStaticHandler(MgfHandlerType::ENTITY_PRISM),
    MgfStaticHandler(MgfHandlerType::ENTITY_FACE_WITH_HOLES),
    MgfStaticHandler(MgfHandlerType::COLOR_SPEC_HELPER),
    MgfStaticHandler(MgfHandlerType::COLOR_MIX_HELPER),
    MgfStaticHandler(MgfHandlerType::COLOR_TEMPERATURE_HELPER),
    MgfStaticHandler(MgfHandlerType::HANDLE_VERTEX),
    MgfStaticHandler(MgfHandlerType::HANDLE_FACE),
    MgfStaticHandler(MgfHandlerType::HANDLE_FACE_WITH_HOLES),
    MgfStaticHandler(MgfHandlerType::HANDLE_SURFACE),
    MgfStaticHandler(MgfHandlerType::HANDLE_COLOR),
    MgfStaticHandler(MgfHandlerType::HANDLE_MATERIAL),
    MgfStaticHandler(MgfHandlerType::HANDLE_TRANSFORM),
    MgfStaticHandler(MgfHandlerType::HANDLE_OBJECT)
};
}

MgfEntityHandler *
mgfHandlerFromType(MgfHandlerType handlerType) {
    const int handlerIndex = static_cast<int>(handlerType);
    if ( handlerIndex < 0 || handlerIndex >= TOTAL_MGF_HANDLER_TYPES ) {
        logFatal(-1, "mgfHandlerFromType", "Unknown MGF handler type %d", handlerIndex);
    }
    return &globalMgfHandlers[handlerIndex];
}

bool
mgfHandlerMatches(const MgfEntityHandler *handler, MgfHandlerType handlerType) {
    return handler != nullptr && handler->type() == handlerType;
}

/**
rayCasterInitialize alternate entity handlers
*/
static void
mgfAlternativeInit(
    MgfEntityHandler *handleCallbacks[TOTAL_NUMBER_OF_ENTITIES],
    MgfContext *context)
{
    unsigned long iNeed = 0;
    unsigned long uNeed = 0;
    int i;

    // Pick up slack
    if ( handleCallbacks[IES] == nullptr) {
        handleCallbacks[IES] = mgfHandlerFromType(MgfHandlerType::DISCARD_UNNEEDED);
    }
    if ( handleCallbacks[INCLUDE] == nullptr ) {
        handleCallbacks[INCLUDE] = mgfHandlerFromType(MgfHandlerType::INCLUDE_FILE);
    }
    if ( handleCallbacks[MgfEntity::SPHERE] == nullptr) {
        handleCallbacks[MgfEntity::SPHERE] = mgfHandlerFromType(MgfHandlerType::ENTITY_SPHERE);
        iNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX;
    } else {
        uNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX | 1L << MgfEntity::TRANSFORM;
    }
    if ( handleCallbacks[MgfEntity::CYLINDER] == nullptr) {
        handleCallbacks[MgfEntity::CYLINDER] = mgfHandlerFromType(MgfHandlerType::ENTITY_CYLINDER);
        iNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX;
    } else {
        uNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX | 1L << MgfEntity::TRANSFORM;
    }
    if ( handleCallbacks[MgfEntity::CONE] == nullptr) {
        handleCallbacks[MgfEntity::CONE] = mgfHandlerFromType(MgfHandlerType::ENTITY_CONE);
        iNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX;
    } else {
        uNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX | 1L << MgfEntity::TRANSFORM;
    }
    if ( handleCallbacks[MgfEntity::RING] == nullptr) {
        handleCallbacks[MgfEntity::RING] = mgfHandlerFromType(MgfHandlerType::ENTITY_RING);
        iNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::MGF_NORMAL | 1L << MgfEntity::VERTEX;
    } else {
        uNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::MGF_NORMAL | 1L << MgfEntity::VERTEX | 1L << MgfEntity::TRANSFORM;
    }
    if ( handleCallbacks[MgfEntity::PRISM] == nullptr) {
        handleCallbacks[MgfEntity::PRISM] = mgfHandlerFromType(MgfHandlerType::ENTITY_PRISM);
        iNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX;
    } else {
        uNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX | 1L << MgfEntity::TRANSFORM;
    }
    if ( handleCallbacks[MgfEntity::TORUS] == nullptr) {
        handleCallbacks[MgfEntity::TORUS] = mgfHandlerFromType(MgfHandlerType::ENTITY_TORUS);
        iNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::MGF_NORMAL | 1L << MgfEntity::VERTEX;
    } else {
        uNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::MGF_NORMAL | 1L << MgfEntity::VERTEX | 1L << MgfEntity::TRANSFORM;
    }
    if ( handleCallbacks[MgfEntity::FACE] == nullptr) {
        handleCallbacks[MgfEntity::FACE] = handleCallbacks[MgfEntity::FACE_WITH_HOLES];
    } else if ( handleCallbacks[MgfEntity::FACE_WITH_HOLES] == nullptr) {
        handleCallbacks[MgfEntity::FACE_WITH_HOLES] = mgfHandlerFromType(MgfHandlerType::ENTITY_FACE_WITH_HOLES);
    }
    if ( handleCallbacks[MgfEntity::COLOR] != nullptr) {
        if ( handleCallbacks[MgfEntity::C_MIX] == nullptr) {
            handleCallbacks[MgfEntity::C_MIX] = mgfHandlerFromType(MgfHandlerType::COLOR_MIX_HELPER);
            iNeed |= 1L << MgfEntity::COLOR | 1L << MgfEntity::CXY | 1L << MgfEntity::C_SPEC | 1L << MgfEntity::C_MIX | 1L << MgfEntity::CCT;
        }
        if ( handleCallbacks[MgfEntity::C_SPEC] == nullptr) {
            handleCallbacks[MgfEntity::C_SPEC] = mgfHandlerFromType(MgfHandlerType::COLOR_SPEC_HELPER);
            iNeed |= 1L << MgfEntity::COLOR | 1L << MgfEntity::CXY | 1L << MgfEntity::C_SPEC | 1L << MgfEntity::C_MIX | 1L << MgfEntity::CCT;
        }
        if ( handleCallbacks[MgfEntity::CCT] == nullptr) {
            handleCallbacks[MgfEntity::CCT] = mgfHandlerFromType(MgfHandlerType::COLOR_TEMPERATURE_HELPER);
            iNeed |= 1L << MgfEntity::COLOR | 1L << MgfEntity::CXY | 1L << MgfEntity::C_SPEC | 1L << MgfEntity::C_MIX | 1L << MgfEntity::CCT;
        }
    }

    // Check for consistency
    if ( handleCallbacks[MgfEntity::FACE] != nullptr) {
        uNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX | 1L << MgfEntity::TRANSFORM;
    }
    if ( handleCallbacks[MgfEntity::CXY] != nullptr || handleCallbacks[MgfEntity::C_SPEC] != nullptr ||
         handleCallbacks[MgfEntity::C_MIX] != nullptr) {
        uNeed |= 1L << MgfEntity::COLOR;
    }
    if ( handleCallbacks[MgfEntity::RD] != nullptr || handleCallbacks[MgfEntity::TD] != nullptr ||
         handleCallbacks[MgfEntity::IR] != nullptr ||
         handleCallbacks[MgfEntity::ED] != nullptr ||
         handleCallbacks[MgfEntity::RS] != nullptr ||
         handleCallbacks[MgfEntity::TS] != nullptr ||
         handleCallbacks[MgfEntity::SIDES] != nullptr) {
        uNeed |= 1L << MgfEntity::MGF_MATERIAL;
    }
    for ( i = 0; i < TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( uNeed & 1L << i && handleCallbacks[i] == nullptr) {
            java::lang::System::err.printf("Missing support for \"%s\" entity\n",
                context->entityNames[i]);
            exit(1);
        }
    }

    // Add support as needed
    if ( iNeed & 1L << MgfEntity::VERTEX && !mgfHandlerMatches(handleCallbacks[MgfEntity::VERTEX], MgfHandlerType::HANDLE_VERTEX) ) {
        context->supportCallbacks[MgfEntity::VERTEX] = mgfHandlerFromType(MgfHandlerType::HANDLE_VERTEX);
    }
    if ( iNeed & 1L << MgfEntity::MGF_POINT && !mgfHandlerMatches(handleCallbacks[MgfEntity::MGF_POINT], MgfHandlerType::HANDLE_VERTEX) ) {
        context->supportCallbacks[MgfEntity::MGF_POINT] = mgfHandlerFromType(MgfHandlerType::HANDLE_VERTEX);
    }
    if ( iNeed & 1L << MgfEntity::MGF_NORMAL && !mgfHandlerMatches(handleCallbacks[MgfEntity::MGF_NORMAL], MgfHandlerType::HANDLE_VERTEX) ) {
        context->supportCallbacks[MgfEntity::MGF_NORMAL] = mgfHandlerFromType(MgfHandlerType::HANDLE_VERTEX);
    }
    if ( iNeed & 1L << MgfEntity::COLOR && !mgfHandlerMatches(handleCallbacks[MgfEntity::COLOR], MgfHandlerType::HANDLE_COLOR) ) {
        context->supportCallbacks[MgfEntity::COLOR] = mgfHandlerFromType(MgfHandlerType::HANDLE_COLOR);
    }
    if ( iNeed & 1L << MgfEntity::CXY && !mgfHandlerMatches(handleCallbacks[MgfEntity::CXY], MgfHandlerType::HANDLE_COLOR) ) {
        context->supportCallbacks[MgfEntity::CXY] = mgfHandlerFromType(MgfHandlerType::HANDLE_COLOR);
    }
    if ( iNeed & 1L << MgfEntity::C_SPEC && !mgfHandlerMatches(handleCallbacks[MgfEntity::C_SPEC], MgfHandlerType::HANDLE_COLOR) ) {
        context->supportCallbacks[MgfEntity::C_SPEC] = mgfHandlerFromType(MgfHandlerType::HANDLE_COLOR);
    }
    if ( iNeed & 1L << MgfEntity::C_MIX && !mgfHandlerMatches(handleCallbacks[MgfEntity::C_MIX], MgfHandlerType::HANDLE_COLOR) ) {
        context->supportCallbacks[MgfEntity::C_MIX] = mgfHandlerFromType(MgfHandlerType::HANDLE_COLOR);
    }
    if ( iNeed & 1L << MgfEntity::CCT && !mgfHandlerMatches(handleCallbacks[MgfEntity::CCT], MgfHandlerType::HANDLE_COLOR) ) {
        context->supportCallbacks[MgfEntity::CCT] = mgfHandlerFromType(MgfHandlerType::HANDLE_COLOR);
    }

    // Discard remaining entities
    for ( i = 0; i < TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( handleCallbacks[i] == nullptr) {
            handleCallbacks[i] = mgfHandlerFromType(MgfHandlerType::DISCARD_UNNEEDED);
        }
    }
}

static void
initMgf(MgfContext *context) {
    // Related to MgfColorContext
    context->handleCallbacks[MgfEntity::COLOR] = mgfHandlerFromType(MgfHandlerType::HANDLE_COLOR);
    context->handleCallbacks[MgfEntity::CXY] = mgfHandlerFromType(MgfHandlerType::HANDLE_COLOR);
    context->handleCallbacks[MgfEntity::C_MIX] = mgfHandlerFromType(MgfHandlerType::HANDLE_COLOR);

    // Related to MgfMaterialContext
    context->handleCallbacks[MgfEntity::MGF_MATERIAL] = mgfHandlerFromType(MgfHandlerType::HANDLE_MATERIAL);
    context->handleCallbacks[MgfEntity::ED] = mgfHandlerFromType(MgfHandlerType::HANDLE_MATERIAL);
    context->handleCallbacks[MgfEntity::IR] = mgfHandlerFromType(MgfHandlerType::HANDLE_MATERIAL);
    context->handleCallbacks[MgfEntity::RD] = mgfHandlerFromType(MgfHandlerType::HANDLE_MATERIAL);
    context->handleCallbacks[MgfEntity::RS] = mgfHandlerFromType(MgfHandlerType::HANDLE_MATERIAL);
    context->handleCallbacks[MgfEntity::SIDES] = mgfHandlerFromType(MgfHandlerType::HANDLE_MATERIAL);
    context->handleCallbacks[MgfEntity::TD] = mgfHandlerFromType(MgfHandlerType::HANDLE_MATERIAL);
    context->handleCallbacks[MgfEntity::TS] = mgfHandlerFromType(MgfHandlerType::HANDLE_MATERIAL);

    // Related to MgfTransformContext
    context->handleCallbacks[MgfEntity::TRANSFORM] = mgfHandlerFromType(MgfHandlerType::HANDLE_TRANSFORM);

    // Related to object, no explicit context
    context->handleCallbacks[MgfEntity::OBJECT] = mgfHandlerFromType(MgfHandlerType::HANDLE_OBJECT);

    // Related to geometry elements, no explicit context
    context->handleCallbacks[MgfEntity::FACE] = mgfHandlerFromType(MgfHandlerType::HANDLE_FACE);
    context->handleCallbacks[MgfEntity::FACE_WITH_HOLES] = mgfHandlerFromType(MgfHandlerType::HANDLE_FACE_WITH_HOLES);
    context->handleCallbacks[MgfEntity::VERTEX] = mgfHandlerFromType(MgfHandlerType::HANDLE_VERTEX);
    context->handleCallbacks[MgfEntity::MGF_POINT] = mgfHandlerFromType(MgfHandlerType::HANDLE_VERTEX);
    context->handleCallbacks[MgfEntity::MGF_NORMAL] = mgfHandlerFromType(MgfHandlerType::HANDLE_VERTEX);
    context->handleCallbacks[MgfEntity::SPHERE] = mgfHandlerFromType(MgfHandlerType::HANDLE_SURFACE);
    context->handleCallbacks[MgfEntity::TORUS] = mgfHandlerFromType(MgfHandlerType::HANDLE_SURFACE);
    context->handleCallbacks[MgfEntity::RING] = mgfHandlerFromType(MgfHandlerType::HANDLE_SURFACE);
    context->handleCallbacks[MgfEntity::CYLINDER] = mgfHandlerFromType(MgfHandlerType::HANDLE_SURFACE);
    context->handleCallbacks[MgfEntity::CONE] = mgfHandlerFromType(MgfHandlerType::HANDLE_SURFACE);
    context->handleCallbacks[MgfEntity::PRISM] = mgfHandlerFromType(MgfHandlerType::HANDLE_SURFACE);

    mgfAlternativeInit(context->handleCallbacks, context);
}

static MgfModel *
mgfBuildModel(MgfContext *context) {
    if ( context == nullptr ) {
        return nullptr;
    }

    if ( context->model == nullptr ) {
        context->model = new MgfModel();
    }

    MgfModel *model = context->model;
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
Reads in a mgf file. The result is that the global variables
context->geometries and context->materials are filled in, and a MgfModel
snapshot with parser outputs/state pointers is returned.

Note: this is an implementation of MGF file format with major version number 2.
*/
MgfModel *
readMgf(const char *filename, MgfContext *context) {
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

    mgfObjectNewSurface(context);

    MgfReaderContext mgfReaderContext{};
    int status;
    if ( filename[0] == '#' ) {
        status = mgfOpen(&mgfReaderContext, nullptr, context);
    } else {
        status = mgfOpen(&mgfReaderContext, filename, context);
    }
    if ( status ) {
        doError(context->errorCodeMessages[status], context);
    } else {
        while ( mgfReadNextLine(context) > 0 && !status ) {
            status = mgfParseCurrentLine(context);
            if ( status ) {
                doError(context->errorCodeMessages[status], context);
            }
        }
        mgfClose(context);
    }
    mgfClear(context);

    if ( context->inSurface ) {
        mgfObjectSurfaceDone(context);
    }
    context->geometries = context->currentGeometryList;

    return mgfBuildModel(context);
}

void
mgfFreeMemory(MgfContext *context) {
    if ( context->currentGeometryList != nullptr ) {
        java::lang::System::out.printf("Freeing %ld geometries\n", context->currentGeometryList->size());
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
        java::lang::System::out.printf("  - MeshSurfaces: %ld\n", surfaces);
        java::lang::System::out.printf("  - Patch sets: %ld\n", patchSets);
        java::lang::System::out.printf("  - Compounds: %ld\n", compounds);
        java::lang::System::out.printf("    . Children: %ld\n", compoundChildren);
        java::lang::System::out.printf("    . Inner children: %ld\n", innerCompoundChildren);
        java::lang::System::out.printf("  - Unknowns: %ld\n", unknowns);
        java::lang::System::out.flush();
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

    mgfObjectFreeMemory();
    mgfTransformFreeMemory();
    mgfLookUpFreeMemory();
}
