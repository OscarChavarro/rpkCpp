#include <cctype>
#include <cstring>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "scene/scene.h"
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

static VectorOctreeNode *globalPointsOctree = nullptr;
static VectorOctreeNode *globalNormalsOctree = nullptr;

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
    char *argv[MGF_MAXIMUM_ARGUMENT_COUNT];
    char *cp;
    char *cp2;
    char **ap;

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
    initColorContextTables();
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
Sets a flag indicating whether all surfaces should be considered
1-sided (the best for "good" models) or not.

When the argument is false, surfaces are considered two-sided unless
explicit specified 1-sided in the mgf file. When the argument is true,
the specified single side flag in the mgf file are ignored and all surfaces
considered 1-sided. This may yield a significant speedup for "good"
models (containing only solids with consistently defined
face normals and vertices in counter clockwise order as seen from the
normal direction)
*/
static void
mgfSetIgnoreSingleSide(bool yesno) {
    GLOBAL_mgf_allSurfacesSided = yesno;
}

/**
If yesno is true, all materials will be converted to be GLOBAL_mgf_monochrome.
*/
static void
mgfSetMonochrome(bool yesno, MgfContext *context) {
    context->monochrome = yesno;
}

/**
Discard unneeded/unwanted entity
*/
static int
mgfDiscardUnNeededEntity(int /*ac*/, char ** /*av*/, MgfContext * /*context*/) {
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
    char *newAv[NUMBER_OF_SPECTRAL_SAMPLES + 4];
    double sf;
    int i;

    if ( context->handleCallbacks[MGF_ENTITY_C_SPEC] != handleColorEntity ) {
        snprintf(wl[0], 6, "%d", COLOR_MINIMUM_WAVE_LENGTH);
        snprintf(wl[1], 6, "%d", COLOR_MAXIMUM_WAVE_LENGTH);
        newAv[0] = context->entityNames[MGF_ENTITY_C_SPEC];
        newAv[1] = wl[0];
        newAv[2] = wl[1];
        sf = (double)NUMBER_OF_SPECTRAL_SAMPLES / (double)GLOBAL_mgf_currentColor->spectralStraightSum;
        for ( i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            snprintf(buffer[i], 24, "%.4f", sf * GLOBAL_mgf_currentColor->straightSamples[i]);
            newAv[i + 3] = buffer[i];
        }
        newAv[NUMBER_OF_SPECTRAL_SAMPLES + 3] = nullptr;
        if ((i = mgfHandle(MGF_ENTITY_C_SPEC, NUMBER_OF_SPECTRAL_SAMPLES + 3, newAv, context)) != MGF_OK ) {
            return i;
        }
    }
    return MGF_OK;
}

/**
Put out current xy chromatic values
*/
static int
mgfPutCxy(MgfContext *context)
{
    static char xBuffer[24];
    static char yBuffer[24];
    static char *cCom[4] = {context->entityNames[MGF_ENTITY_CXY], xBuffer, yBuffer};

    snprintf(xBuffer, 24, "%.4f", GLOBAL_mgf_currentColor->cx);
    snprintf(yBuffer, 24, "%.4f", GLOBAL_mgf_currentColor->cy);
    return mgfHandle(MGF_ENTITY_CXY, 3, cCom, context);
}

/**
Handle spectral color
*/
static int
mgfECSpec(int /*ac*/, char ** /*av*/, MgfContext *context) {
    // Convert to xy chromaticity
    mgfContextFixColorRepresentation(GLOBAL_mgf_currentColor, COLOR_XY_IS_SET_FLAG);
    // If it's really their handler, use it
    if ( context->handleCallbacks[MGF_ENTITY_CXY] != handleColorEntity ) {
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
mgfECMix(int /*ac*/, char ** /*av*/, MgfContext *context) {
    if ( context->handleCallbacks[MGF_ENTITY_C_SPEC] == mgfECSpec ) {
        mgfContextFixColorRepresentation(GLOBAL_mgf_currentColor, COLOR_XY_IS_SET_FLAG);
    } else if ( GLOBAL_mgf_currentColor->flags & COLOR_DEFINED_WITH_SPECTRUM_FLAG ) {
        return mgfPutCSpec(context);
    }
    if ( context->handleCallbacks[MGF_ENTITY_CXY] != handleColorEntity ) {
        return mgfPutCxy(context);
    }
    return MGF_OK;
}

/**
Handle color temperature
*/
static int
mgfColorTemperature(int /*ac*/, char ** /*av*/, MgfContext *context)
{
    // Logic is similar to mgfECMix here.  Support handler has already
    // converted temperature to spectral color.  Put it out as such
    // if they support it, otherwise convert to xy chromaticity and
    // put it out if they handle it
    if ( context->handleCallbacks[MGF_ENTITY_C_SPEC] != mgfECSpec ) {
        return mgfPutCSpec(context);
    }
    mgfContextFixColorRepresentation(GLOBAL_mgf_currentColor, COLOR_XY_IS_SET_FLAG);
    if ( context->handleCallbacks[MGF_ENTITY_CXY] != handleColorEntity ) {
        return mgfPutCxy(context);
    }
    return MGF_OK;
}

static int
handleIncludedFile(int ac, char **av, MgfContext *context)
{
    char *transformArgument[MGF_MAXIMUM_ARGUMENT_COUNT];
    MgfReaderContext readerContext{};
    MgfTransformContext *xf_orig = GLOBAL_mgf_xfContext;

    if ( ac < 2 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }

    int rv = mgfOpen(&readerContext, av[1], context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    if ( ac > 2 ) {
        transformArgument[0] = context->entityNames[MGF_ENTITY_XF];
        for ( int i = 1; i < ac - 1; i++ ) {
            transformArgument[i] = av[i + 1];
        }
        transformArgument[ac - 1] = nullptr;
        rv = mgfHandle(MGF_ENTITY_XF, ac - 1, transformArgument, context);
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
            rv = mgfHandle(MGF_ENTITY_XF, 1, transformArgument, context);
            if ( rv != MGF_OK ) {
                mgfClose(context);
                return rv;
            }
        }
    } while ( GLOBAL_mgf_xfContext != xf_orig );
    mgfClose(context);
    return MGF_OK;
}

/**
rayCasterInitialize alternate entity handlers
*/
static void
mgfAlternativeInit(int (*handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int, char **, MgfContext *), MgfContext *context) {
    unsigned long iNeed = 0;
    unsigned long uNeed = 0;
    int i;

    // Pick up slack
    if ( handleCallbacks[MG_E_IES] == nullptr) {
        handleCallbacks[MG_E_IES] = mgfDiscardUnNeededEntity;
    }
    if ( handleCallbacks[MG_E_INCLUDE] == nullptr) {
        handleCallbacks[MG_E_INCLUDE] = handleIncludedFile;
    }
    if ( handleCallbacks[MGF_ENTITY_SPHERE] == nullptr) {
        handleCallbacks[MGF_ENTITY_SPHERE] = mgfEntitySphere;
        iNeed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX;
    } else {
        uNeed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_CYLINDER] == nullptr) {
        handleCallbacks[MGF_ENTITY_CYLINDER] = mgfEntityCylinder;
        iNeed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX;
    } else {
        uNeed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_CONE] == nullptr) {
        handleCallbacks[MGF_ENTITY_CONE] = mgfEntityCone;
        iNeed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX;
    } else {
        uNeed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_RING] == nullptr) {
        handleCallbacks[MGF_ENTITY_RING] = mgfEntityRing;
        iNeed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_NORMAL | 1L << MGF_ENTITY_VERTEX;
    } else {
        uNeed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_NORMAL | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_PRISM] == nullptr) {
        handleCallbacks[MGF_ENTITY_PRISM] = mgfEntityPrism;
        iNeed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX;
    } else {
        uNeed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_TORUS] == nullptr) {
        handleCallbacks[MGF_ENTITY_TORUS] = mgfEntityTorus;
        iNeed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_NORMAL | 1L << MGF_ENTITY_VERTEX;
    } else {
        uNeed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_NORMAL | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_FACE] == nullptr) {
        handleCallbacks[MGF_ENTITY_FACE] = handleCallbacks[MGF_ENTITY_FACE_WITH_HOLES];
    } else if ( handleCallbacks[MGF_ENTITY_FACE_WITH_HOLES] == nullptr) {
        handleCallbacks[MGF_ENTITY_FACE_WITH_HOLES] = mgfEntityFaceWithHoles;
    }
    if ( handleCallbacks[MGF_ENTITY_COLOR] != nullptr) {
        if ( handleCallbacks[MGF_ENTITY_C_MIX] == nullptr) {
            handleCallbacks[MGF_ENTITY_C_MIX] = mgfECMix;
            iNeed |= 1L << MGF_ENTITY_COLOR | 1L << MGF_ENTITY_CXY | 1L << MGF_ENTITY_C_SPEC | 1L << MGF_ENTITY_C_MIX | 1L << MGF_ENTITY_CCT;
        }
        if ( handleCallbacks[MGF_ENTITY_C_SPEC] == nullptr) {
            handleCallbacks[MGF_ENTITY_C_SPEC] = mgfECSpec;
            iNeed |= 1L << MGF_ENTITY_COLOR | 1L << MGF_ENTITY_CXY | 1L << MGF_ENTITY_C_SPEC | 1L << MGF_ENTITY_C_MIX | 1L << MGF_ENTITY_CCT;
        }
        if ( handleCallbacks[MGF_ENTITY_CCT] == nullptr) {
            handleCallbacks[MGF_ENTITY_CCT] = mgfColorTemperature;
            iNeed |= 1L << MGF_ENTITY_COLOR | 1L << MGF_ENTITY_CXY | 1L << MGF_ENTITY_C_SPEC | 1L << MGF_ENTITY_C_MIX | 1L << MGF_ENTITY_CCT;
        }
    }

    // Check for consistency
    if ( handleCallbacks[MGF_ENTITY_FACE] != nullptr) {
        uNeed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_CXY] != nullptr || handleCallbacks[MGF_ENTITY_C_SPEC] != nullptr ||
         handleCallbacks[MGF_ENTITY_C_MIX] != nullptr) {
        uNeed |= 1L << MGF_ENTITY_COLOR;
    }
    if ( handleCallbacks[MGF_ENTITY_RD] != nullptr || handleCallbacks[MGF_ENTITY_TD] != nullptr ||
         handleCallbacks[MGF_ENTITY_IR] != nullptr ||
         handleCallbacks[MGF_ENTITY_ED] != nullptr ||
         handleCallbacks[MGF_ENTITY_RS] != nullptr ||
         handleCallbacks[MGF_ENTITY_TS] != nullptr ||
         handleCallbacks[MGF_ENTITY_SIDES] != nullptr) {
        uNeed |= 1L << MGF_ENTITY_MATERIAL;
    }
    for ( i = 0; i < MGF_TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( uNeed & 1L << i && handleCallbacks[i] == nullptr) {
            fprintf(stderr, "Missing support for \"%s\" entity\n",
                context->entityNames[i]);
            exit(1);
        }
    }

    // Add support as needed
    if ( iNeed & 1L << MGF_ENTITY_VERTEX && handleCallbacks[MGF_ENTITY_VERTEX] != handleVertexEntity ) {
        context->supportCallbacks[MGF_ENTITY_VERTEX] = handleVertexEntity;
    }
    if ( iNeed & 1L << MGF_ENTITY_POINT && handleCallbacks[MGF_ENTITY_POINT] != handleVertexEntity ) {
        context->supportCallbacks[MGF_ENTITY_POINT] = handleVertexEntity;
    }
    if ( iNeed & 1L << MGF_ENTITY_NORMAL && handleCallbacks[MGF_ENTITY_NORMAL] != handleVertexEntity ) {
        context->supportCallbacks[MGF_ENTITY_NORMAL] = handleVertexEntity;
    }
    if ( iNeed & 1L << MGF_ENTITY_COLOR && handleCallbacks[MGF_ENTITY_COLOR] != handleColorEntity ) {
        context->supportCallbacks[MGF_ENTITY_COLOR] = handleColorEntity;
    }
    if ( iNeed & 1L << MGF_ENTITY_CXY && handleCallbacks[MGF_ENTITY_CXY] != handleColorEntity ) {
        context->supportCallbacks[MGF_ENTITY_CXY] = handleColorEntity;
    }
    if ( iNeed & 1L << MGF_ENTITY_C_SPEC && handleCallbacks[MGF_ENTITY_C_SPEC] != handleColorEntity ) {
        context->supportCallbacks[MGF_ENTITY_C_SPEC] = handleColorEntity;
    }
    if ( iNeed & 1L << MGF_ENTITY_C_MIX && handleCallbacks[MGF_ENTITY_C_MIX] != handleColorEntity ) {
        context->supportCallbacks[MGF_ENTITY_C_MIX] = handleColorEntity;
    }
    if ( iNeed & 1L << MGF_ENTITY_CCT && handleCallbacks[MGF_ENTITY_CCT] != handleColorEntity ) {
        context->supportCallbacks[MGF_ENTITY_CCT] = handleColorEntity;
    }

    // Discard remaining entities
    for ( i = 0; i < MGF_TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( handleCallbacks[i] == nullptr) {
            handleCallbacks[i] = mgfDiscardUnNeededEntity;
        }
    }
}

static void
initMgf(MgfContext *context) {
    // Related to MgfColorContext
    context->handleCallbacks[MGF_ENTITY_COLOR] = handleColorEntity;
    context->handleCallbacks[MGF_ENTITY_CXY] = handleColorEntity;
    context->handleCallbacks[MGF_ENTITY_C_MIX] = handleColorEntity;

    // Related to MgfMaterialContext
    context->handleCallbacks[MGF_ENTITY_MATERIAL] = handleMaterialEntity;
    context->handleCallbacks[MGF_ENTITY_ED] = handleMaterialEntity;
    context->handleCallbacks[MGF_ENTITY_IR] = handleMaterialEntity;
    context->handleCallbacks[MGF_ENTITY_RD] = handleMaterialEntity;
    context->handleCallbacks[MGF_ENTITY_RS] = handleMaterialEntity;
    context->handleCallbacks[MGF_ENTITY_SIDES] = handleMaterialEntity;
    context->handleCallbacks[MGF_ENTITY_TD] = handleMaterialEntity;
    context->handleCallbacks[MGF_ENTITY_TS] = handleMaterialEntity;

    // Related to MgfTransformContext
    context->handleCallbacks[MGF_ENTITY_XF] = handleTransformationEntity;

    // Related to object, no explicit context
    context->handleCallbacks[MGF_ENTITY_OBJECT] = handleObjectEntity;

    // Related to geometry elements, no explicit context
    context->handleCallbacks[MGF_ENTITY_FACE] = handleFaceEntity;
    context->handleCallbacks[MGF_ENTITY_FACE_WITH_HOLES] = handleFaceWithHolesEntity;
    context->handleCallbacks[MGF_ENTITY_VERTEX] = handleVertexEntity;
    context->handleCallbacks[MGF_ENTITY_POINT] = handleVertexEntity;
    context->handleCallbacks[MGF_ENTITY_NORMAL] = handleVertexEntity;
    context->handleCallbacks[MGF_ENTITY_SPHERE] = handleSurfaceEntity;
    context->handleCallbacks[MGF_ENTITY_TORUS] = handleSurfaceEntity;
    context->handleCallbacks[MGF_ENTITY_RING] = handleSurfaceEntity;
    context->handleCallbacks[MGF_ENTITY_CYLINDER] = handleSurfaceEntity;
    context->handleCallbacks[MGF_ENTITY_CONE] = handleSurfaceEntity;
    context->handleCallbacks[MGF_ENTITY_PRISM] = handleSurfaceEntity;

    mgfAlternativeInit(context->handleCallbacks, context);
}

static void
freeLists() {
    if ( GLOBAL_mgf_currentPointList != nullptr ) {
        delete GLOBAL_mgf_currentPointList;
        GLOBAL_mgf_currentPointList = nullptr;
    }

    if ( GLOBAL_mgf_currentNormalList != nullptr ) {
        delete GLOBAL_mgf_currentNormalList;
        GLOBAL_mgf_currentNormalList = nullptr;
    }

    if ( GLOBAL_mgf_currentVertexList != nullptr ) {
        delete GLOBAL_mgf_currentVertexList;
        GLOBAL_mgf_currentVertexList = nullptr;
    }

    if ( GLOBAL_mgf_currentFaceList != nullptr ) {
        delete GLOBAL_mgf_currentFaceList;
        GLOBAL_mgf_currentFaceList = nullptr;
    }
}

/**
Reads in an mgf file. The result is that the global variables
GLOBAL_scene_world and GLOBAL_scene_materials are filled in.

Note: this is an implementation of MGF file format with major version number 2.
*/
void
readMgf(char *filename, MgfContext *context)
{
    mgfSetNrQuartCircDivs(context->numberOfQuarterCircleDivisions);
    mgfSetIgnoreSingleSide(context->singleSided);
    mgfSetMonochrome(context->monochrome, context);

    initMgf(context);

    globalPointsOctree = nullptr;
    globalNormalsOctree = nullptr;
    GLOBAL_mgf_currentGeometryList = new java::ArrayList<Geometry *>();

    if ( GLOBAL_scene_materials == nullptr ) {
        GLOBAL_scene_materials = new java::ArrayList<Material *>();
    }
    context->currentMaterial = &GLOBAL_material_defaultMaterial;

    context->geometryStackPtr = context->geometryStack;

    GLOBAL_mgf_inComplex = false;
    GLOBAL_mgf_inSurface = false;

    newSurface();

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

    if ( GLOBAL_mgf_inSurface ) {
        surfaceDone(context);
    }
    GLOBAL_scene_geometries = GLOBAL_mgf_currentGeometryList;

    if ( globalPointsOctree != nullptr) {
        free(globalPointsOctree);
    }
    if ( globalNormalsOctree != nullptr) {
        free(globalNormalsOctree);
    }
}

void
mgfFreeMemory() {
    printf("Freeing %ld geometries\n", GLOBAL_mgf_currentGeometryList->size());
    long surfaces = 0;
    long patchSets = 0;
    for ( int i = 0; i < GLOBAL_mgf_currentGeometryList->size(); i++ ) {
        if ( GLOBAL_mgf_currentGeometryList->get(i)->className == SURFACE_MESH ) {
            surfaces++;
        }
        if ( GLOBAL_mgf_currentGeometryList->get(i)->className == PATCH_SET ) {
            patchSets++;
        }
    }
    printf("  - Surfaces: %ld\n", surfaces);
    printf("  - Patch sets: %ld\n", patchSets);
    fflush(stdout);

    for ( int i = 0; i < GLOBAL_mgf_currentGeometryList->size(); i++ ) {
        geomDestroy(GLOBAL_mgf_currentGeometryList->get(i));
    }
    delete GLOBAL_mgf_currentGeometryList;
    GLOBAL_mgf_currentGeometryList = nullptr;

    freeLists();
}
