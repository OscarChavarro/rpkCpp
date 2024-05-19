#include <cctype>
#include <cstring>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "io/mgf/vectoroctree.h"
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
mgfReadNextLine(MgfContext *context) {
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
static int
mgfParseCurrentLine(MgfContext *context) {
    char buffer[MGF_MAXIMUM_INPUT_LINE_LENGTH];
    const char *argv[MGF_MAXIMUM_ARGUMENT_COUNT];
    char *cp;
    const char *cp2;
    const char **ap;

    // Copy line, removing escape chars
    cp = buffer;
    cp2 = context->readerContext->inputLine;
    while ( (*cp++ = *cp2++) ) {
        if ( cp2[0] == '\n' && cp2[-1] == '\\' ) {
            cp--;
        }
    }
    cp = buffer;
    ap = argv; // Break into words
    for ( ;; ) {
        while ( isspace(*cp) ) {
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
    return MGF_OK;
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
    double sf;

    if ( context->handleCallbacks[MgfEntity::C_SPEC] != (HandleCallBack)handleColorEntity ) {
        snprintf(wl[0], 6, "%d", COLOR_MINIMUM_WAVE_LENGTH);
        snprintf(wl[1], 6, "%d", COLOR_MAXIMUM_WAVE_LENGTH);
        newAv[0] = context->entityNames[MgfEntity::C_SPEC];
        newAv[1] = wl[0];
        newAv[2] = wl[1];
        sf = (double)NUMBER_OF_SPECTRAL_SAMPLES / (double)(context->currentColor->spectralStraightSum);
        for ( int i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            snprintf(buffer[i], 24, "%.4f", sf * context->currentColor->straightSamples[i]);
            newAv[i + 3] = buffer[i];
        }
        newAv[NUMBER_OF_SPECTRAL_SAMPLES + 3] = nullptr;
        int status = mgfHandle(MgfEntity::C_SPEC, NUMBER_OF_SPECTRAL_SAMPLES + 3, newAv, context);
        if ( status != MGF_OK ) {
            return status;
        }
    }
    return MGF_OK;
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
    if ( context->handleCallbacks[MgfEntity::CXY] != (HandleCallBack)handleColorEntity ) {
        return mgfPutCxy(context);
    }
    return MGF_OK;
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
    if ( context->handleCallbacks[MgfEntity::C_SPEC] == (HandleCallBack)mgfECSpec ) {
        context->currentColor->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
    } else if ( context->currentColor->flags & COLOR_DEFINED_WITH_SPECTRUM_FLAG ) {
        return mgfPutCSpec(context);
    }
    if ( context->handleCallbacks[MgfEntity::CXY] != (HandleCallBack)handleColorEntity ) {
        return mgfPutCxy(context);
    }
    return MGF_OK;
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
    if ( context->handleCallbacks[MgfEntity::C_SPEC] != (HandleCallBack)mgfECSpec ) {
        return mgfPutCSpec(context);
    }
    context->currentColor->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
    if ( context->handleCallbacks[MgfEntity::CXY] != (HandleCallBack)handleColorEntity ) {
        return mgfPutCxy(context);
    }
    return MGF_OK;
}

static int
handleIncludedFile(int ac, const char **av, MgfContext *context) {
    const char *transformArgument[MGF_MAXIMUM_ARGUMENT_COUNT];
    MgfReaderContext readerContext{};
    const MgfTransformContext *originTransform = context->transformContext;

    if ( ac < 2 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }

    int rv = mgfOpen(&readerContext, av[1], context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    if ( ac > 2 ) {
        transformArgument[0] = context->entityNames[MgfEntity::TRANSFORM];
        for ( int i = 1; i < ac - 1; i++ ) {
            transformArgument[i] = av[i + 1];
        }
        transformArgument[ac - 1] = nullptr;
        rv = mgfHandle(MgfEntity::TRANSFORM, ac - 1, transformArgument, context);
        if ( rv != MGF_OK ) {
            mgfClose(context);
            return rv;
        }
    }
    do {
        while ( (rv = mgfReadNextLine(context)) > 0 ) {
            if ( rv >= MGF_MAXIMUM_INPUT_LINE_LENGTH - 1 ) {
                fprintf(stderr, "%s: %d: %s\n", readerContext.fileName,
                        readerContext.lineNumber, context->errorCodeMessages[MGF_ERROR_LINE_TOO_LONG]);
                mgfClose(context);
                return MGF_ERROR_IN_INCLUDED_FILE;
            }
            rv = mgfParseCurrentLine(context);
            if ( rv != MGF_OK ) {
                fprintf(stderr, "%s: %d: %s:\n%s", readerContext.fileName,
                        readerContext.lineNumber, context->errorCodeMessages[rv],
                        readerContext.inputLine);
                mgfClose(context);
                return MGF_ERROR_IN_INCLUDED_FILE;
            }
        }
        if ( ac > 2 ) {
            rv = mgfHandle(MgfEntity::TRANSFORM, 1, transformArgument, context);
            if ( rv != MGF_OK ) {
                mgfClose(context);
                return rv;
            }
        }
    } while ( context->transformContext != originTransform );
    mgfClose(context);
    return MGF_OK;
}

/**
rayCasterInitialize alternate entity handlers
*/
static void
mgfAlternativeInit(
    int (*handleCallbacks[TOTAL_NUMBER_OF_ENTITIES])(int, const char **, MgfContext *),
    MgfContext *context)
{
    unsigned long iNeed = 0;
    unsigned long uNeed = 0;
    int i;

    // Pick up slack
    if ( handleCallbacks[IES] == nullptr) {
        handleCallbacks[IES] = mgfDiscardUnNeededEntity;
    }
    if ( handleCallbacks[INCLUDE] == nullptr ) {
        handleCallbacks[INCLUDE] = handleIncludedFile;
    }
    if ( handleCallbacks[MgfEntity::SPHERE] == nullptr) {
        handleCallbacks[MgfEntity::SPHERE] = mgfEntitySphere;
        iNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX;
    } else {
        uNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX | 1L << MgfEntity::TRANSFORM;
    }
    if ( handleCallbacks[MgfEntity::CYLINDER] == nullptr) {
        handleCallbacks[MgfEntity::CYLINDER] = mgfEntityCylinder;
        iNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX;
    } else {
        uNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX | 1L << MgfEntity::TRANSFORM;
    }
    if ( handleCallbacks[MgfEntity::CONE] == nullptr) {
        handleCallbacks[MgfEntity::CONE] = mgfEntityCone;
        iNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX;
    } else {
        uNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX | 1L << MgfEntity::TRANSFORM;
    }
    if ( handleCallbacks[MgfEntity::RING] == nullptr) {
        handleCallbacks[MgfEntity::RING] = mgfEntityRing;
        iNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::MGF_NORMAL | 1L << MgfEntity::VERTEX;
    } else {
        uNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::MGF_NORMAL | 1L << MgfEntity::VERTEX | 1L << MgfEntity::TRANSFORM;
    }
    if ( handleCallbacks[MgfEntity::PRISM] == nullptr) {
        handleCallbacks[MgfEntity::PRISM] = mgfEntityPrism;
        iNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX;
    } else {
        uNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::VERTEX | 1L << MgfEntity::TRANSFORM;
    }
    if ( handleCallbacks[MgfEntity::TORUS] == nullptr) {
        handleCallbacks[MgfEntity::TORUS] = mgfEntityTorus;
        iNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::MGF_NORMAL | 1L << MgfEntity::VERTEX;
    } else {
        uNeed |= 1L << MgfEntity::MGF_POINT | 1L << MgfEntity::MGF_NORMAL | 1L << MgfEntity::VERTEX | 1L << MgfEntity::TRANSFORM;
    }
    if ( handleCallbacks[MgfEntity::FACE] == nullptr) {
        handleCallbacks[MgfEntity::FACE] = handleCallbacks[MgfEntity::FACE_WITH_HOLES];
    } else if ( handleCallbacks[MgfEntity::FACE_WITH_HOLES] == nullptr) {
        handleCallbacks[MgfEntity::FACE_WITH_HOLES] = mgfEntityFaceWithHoles;
    }
    if ( handleCallbacks[MgfEntity::COLOR] != nullptr) {
        if ( handleCallbacks[MgfEntity::C_MIX] == nullptr) {
            handleCallbacks[MgfEntity::C_MIX] = mgfECMix;
            iNeed |= 1L << MgfEntity::COLOR | 1L << MgfEntity::CXY | 1L << MgfEntity::C_SPEC | 1L << MgfEntity::C_MIX | 1L << MgfEntity::CCT;
        }
        if ( handleCallbacks[MgfEntity::C_SPEC] == nullptr) {
            handleCallbacks[MgfEntity::C_SPEC] = mgfECSpec;
            iNeed |= 1L << MgfEntity::COLOR | 1L << MgfEntity::CXY | 1L << MgfEntity::C_SPEC | 1L << MgfEntity::C_MIX | 1L << MgfEntity::CCT;
        }
        if ( handleCallbacks[MgfEntity::CCT] == nullptr) {
            handleCallbacks[MgfEntity::CCT] = mgfColorTemperature;
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
            fprintf(stderr, "Missing support for \"%s\" entity\n",
                context->entityNames[i]);
            exit(1);
        }
    }

    // Add support as needed
    if ( iNeed & 1L << MgfEntity::VERTEX && handleCallbacks[MgfEntity::VERTEX] != (HandleCallBack)handleVertexEntity ) {
        context->supportCallbacks[MgfEntity::VERTEX] = handleVertexEntity;
    }
    if ( iNeed & 1L << MgfEntity::MGF_POINT && handleCallbacks[MgfEntity::MGF_POINT] != (HandleCallBack)handleVertexEntity ) {
        context->supportCallbacks[MgfEntity::MGF_POINT] = handleVertexEntity;
    }
    if ( iNeed & 1L << MgfEntity::MGF_NORMAL && handleCallbacks[MgfEntity::MGF_NORMAL] != (HandleCallBack)handleVertexEntity ) {
        context->supportCallbacks[MgfEntity::MGF_NORMAL] = handleVertexEntity;
    }
    if ( iNeed & 1L << MgfEntity::COLOR && handleCallbacks[MgfEntity::COLOR] != (HandleCallBack)handleColorEntity ) {
        context->supportCallbacks[MgfEntity::COLOR] = handleColorEntity;
    }
    if ( iNeed & 1L << MgfEntity::CXY && handleCallbacks[MgfEntity::CXY] != (HandleCallBack)handleColorEntity ) {
        context->supportCallbacks[MgfEntity::CXY] = handleColorEntity;
    }
    if ( iNeed & 1L << MgfEntity::C_SPEC && handleCallbacks[MgfEntity::C_SPEC] != (HandleCallBack)handleColorEntity ) {
        context->supportCallbacks[MgfEntity::C_SPEC] = handleColorEntity;
    }
    if ( iNeed & 1L << MgfEntity::C_MIX && handleCallbacks[MgfEntity::C_MIX] != (HandleCallBack)handleColorEntity ) {
        context->supportCallbacks[MgfEntity::C_MIX] = handleColorEntity;
    }
    if ( iNeed & 1L << MgfEntity::CCT && handleCallbacks[MgfEntity::CCT] != (HandleCallBack)handleColorEntity ) {
        context->supportCallbacks[MgfEntity::CCT] = handleColorEntity;
    }

    // Discard remaining entities
    for ( i = 0; i < TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( handleCallbacks[i] == nullptr) {
            handleCallbacks[i] = mgfDiscardUnNeededEntity;
        }
    }
}

static void
initMgf(MgfContext *context) {
    // Related to MgfColorContext
    context->handleCallbacks[MgfEntity::COLOR] = handleColorEntity;
    context->handleCallbacks[MgfEntity::CXY] = handleColorEntity;
    context->handleCallbacks[MgfEntity::C_MIX] = handleColorEntity;

    // Related to MgfMaterialContext
    context->handleCallbacks[MgfEntity::MGF_MATERIAL] = handleMaterialEntity;
    context->handleCallbacks[MgfEntity::ED] = handleMaterialEntity;
    context->handleCallbacks[MgfEntity::IR] = handleMaterialEntity;
    context->handleCallbacks[MgfEntity::RD] = handleMaterialEntity;
    context->handleCallbacks[MgfEntity::RS] = handleMaterialEntity;
    context->handleCallbacks[MgfEntity::SIDES] = handleMaterialEntity;
    context->handleCallbacks[MgfEntity::TD] = handleMaterialEntity;
    context->handleCallbacks[MgfEntity::TS] = handleMaterialEntity;

    // Related to MgfTransformContext
    context->handleCallbacks[MgfEntity::TRANSFORM] = handleTransformationEntity;

    // Related to object, no explicit context
    context->handleCallbacks[MgfEntity::OBJECT] = handleObjectEntity;

    // Related to geometry elements, no explicit context
    context->handleCallbacks[MgfEntity::FACE] = handleFaceEntity;
    context->handleCallbacks[MgfEntity::FACE_WITH_HOLES] = handleFaceWithHolesEntity;
    context->handleCallbacks[MgfEntity::VERTEX] = handleVertexEntity;
    context->handleCallbacks[MgfEntity::MGF_POINT] = handleVertexEntity;
    context->handleCallbacks[MgfEntity::MGF_NORMAL] = handleVertexEntity;
    context->handleCallbacks[MgfEntity::SPHERE] = handleSurfaceEntity;
    context->handleCallbacks[MgfEntity::TORUS] = handleSurfaceEntity;
    context->handleCallbacks[MgfEntity::RING] = handleSurfaceEntity;
    context->handleCallbacks[MgfEntity::CYLINDER] = handleSurfaceEntity;
    context->handleCallbacks[MgfEntity::CONE] = handleSurfaceEntity;
    context->handleCallbacks[MgfEntity::PRISM] = handleSurfaceEntity;

    mgfAlternativeInit(context->handleCallbacks, context);
}

/**
Reads in an mgf file. The result is that the global variables
context->geometries and context->materials are filled in.

Note: this is an implementation of MGF file format with major version number 2.
*/
void
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
}

void
mgfFreeMemory(MgfContext *context) {
    printf("Freeing %ld geometries\n", context->currentGeometryList->size());
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
            // Note that this creation is being done as a Geometry parent type!
            if ( geometry->compoundData->children != nullptr ) {
                compoundChildren += geometry->compoundData->children->size();
            }
            compounds++;
        } else {
            unknowns++;
        }
    }
    printf("  - MeshSurfaces: %ld\n", surfaces);
    printf("  - Patch sets: %ld\n", patchSets);
    printf("  - Compounds: %ld\n", compounds);
    printf("    . Children: %ld\n", compoundChildren);
    printf("    . Inner children: %ld\n", innerCompoundChildren);
    printf("  - Unknowns: %ld\n", unknowns);
    fflush(stdout);

    for ( int i = 0; i < context->allGeometries->size(); i++ ) {
        delete context->allGeometries->get(i);
    }

    delete context->currentGeometryList;
    context->currentGeometryList = nullptr;

    if ( context->materials != nullptr ) {
        for ( int i = 0; i < context->materials->size(); i++ ) {
            delete context->materials->get(i);
        }
        delete context->materials;
    }

    mgfObjectFreeMemory();
    mgfTransformFreeMemory();
    mgfLookUpFreeMemory();
}
