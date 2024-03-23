#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "scene/scene.h"
#include "io/mgf/parser.h"
#include "io/mgf/vectoroctree.h"
#include "io/mgf/fileopts.h"
#include "io/mgf/mgfHandlerGeometry.h"
#include "io/mgf/mgfHandlerTransform.h"
#include "io/mgf/mgfHandlerObject.h"
#include "io/mgf/mgfGeometry.h"
#include "io/mgf/MgfColorContext.h"
#include "io/mgf/mgfHandlerMaterial.h"
#include "io/mgf/MgfTransformContext.h"
#include "io/mgf/readmgf.h"

static VectorOctreeNode *globalPointsOctree = nullptr;
static VectorOctreeNode *globalNormalsOctree = nullptr;

static int
handleUnknownEntity(int /*argc*/, char ** /*argv*/) {
    doWarning("unknown entity");

    return MGF_OK;
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

    GLOBAL_mgf_divisionsPerQuarterCircle = divs;
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
If yesno is true, all materials will be converted to be GLOBAL_fileOptions_monochrome.
*/
static void
mgfSetMonochrome(int yesno) {
    GLOBAL_fileOptions_monochrome = yesno;
}

/**
Discard unneeded/unwanted entity
*/
static int
mgfDiscardUnNeededEntity(int /*ac*/, char ** /*av*/, RadianceMethod * /*context*/) {
    return MGF_OK;
}

/**
Put out current color spectrum
*/
static int
mgfPutCSpec(RadianceMethod *context)
{
    char wl[2][6];
    char vbuf[NUMBER_OF_SPECTRAL_SAMPLES][24];
    char *newav[NUMBER_OF_SPECTRAL_SAMPLES + 4];
    double sf;
    int i;

    if ( GLOBAL_mgf_handleCallbacks[MGF_ENTITY_C_SPEC] != handleColorEntity ) {
        snprintf(wl[0], 6, "%d", C_CMINWL);
        snprintf(wl[1], 6, "%d", C_CMAXWL);
        newav[0] = GLOBAL_mgf_entityNames[MGF_ENTITY_C_SPEC];
        newav[1] = wl[0];
        newav[2] = wl[1];
        sf = (double)NUMBER_OF_SPECTRAL_SAMPLES / (double)GLOBAL_mgf_currentColor->spectralStraightSum;
        for ( i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            snprintf(vbuf[i], 24, "%.4f", sf * GLOBAL_mgf_currentColor->straightSamples[i]);
            newav[i + 3] = vbuf[i];
        }
        newav[NUMBER_OF_SPECTRAL_SAMPLES + 3] = nullptr;
        if ((i = mgfHandle(MGF_ENTITY_C_SPEC, NUMBER_OF_SPECTRAL_SAMPLES + 3, newav, context)) != MGF_OK ) {
            return i;
        }
    }
    return MGF_OK;
}

/**
Put out current xy chromaticities
*/
static int
mgfPutCxy(RadianceMethod *context)
{
    static char xbuf[24];
    static char ybuf[24];
    static char *ccom[4] = {GLOBAL_mgf_entityNames[MGF_ENTITY_CXY], xbuf, ybuf};

    snprintf(xbuf, 24, "%.4f", GLOBAL_mgf_currentColor->cx);
    snprintf(ybuf, 24, "%.4f", GLOBAL_mgf_currentColor->cy);
    return mgfHandle(MGF_ENTITY_CXY, 3, ccom, context);
}

/**
Handle spectral color
*/
static int
mgfECSpec(int /*ac*/, char ** /*av*/, RadianceMethod *context) {
    // Convert to xy chromaticity
    mgfContextFixColorRepresentation(GLOBAL_mgf_currentColor, C_CSXY);
    // If it's really their handler, use it
    if ( GLOBAL_mgf_handleCallbacks[MGF_ENTITY_CXY] != handleColorEntity ) {
        return mgfPutCxy(context);
    }
    return MGF_OK;
}

/**
Handle mixing of colors
Contorted logic works as follows:
1. the colors are already mixed in c_hcolor() support function
2. if we would handle a spectral result, make sure it's not
3. if handleColorEntity() would handle a spectral result, don't bother
4. otherwise, make cspec entity and pass it to their handler
5. if we have only xy results, handle it as c_spec() would
*/
static int
mgfECmix(int /*ac*/, char ** /*av*/, RadianceMethod *context) {
    if ( GLOBAL_mgf_handleCallbacks[MGF_ENTITY_C_SPEC] == mgfECSpec ) {
        mgfContextFixColorRepresentation(GLOBAL_mgf_currentColor, C_CSXY);
    } else if ( GLOBAL_mgf_currentColor->flags & C_CDSPEC ) {
        return mgfPutCSpec(context);
    }
    if ( GLOBAL_mgf_handleCallbacks[MGF_ENTITY_CXY] != handleColorEntity ) {
        return mgfPutCxy(context);
    }
    return MGF_OK;
}

/**
Handle color temperature
*/
static int
mgfColorTemperature(int /*ac*/, char ** /*av*/, RadianceMethod *context)
{
    // Logic is similar to mgfECmix here.  Support handler has already
    // converted temperature to spectral color.  Put it out as such
    // if they support it, otherwise convert to xy chromaticity and
    // put it out if they handle it
    if ( GLOBAL_mgf_handleCallbacks[MGF_ENTITY_C_SPEC] != mgfECSpec ) {
        return mgfPutCSpec(context);
    }
    mgfContextFixColorRepresentation(GLOBAL_mgf_currentColor, C_CSXY);
    if ( GLOBAL_mgf_handleCallbacks[MGF_ENTITY_CXY] != handleColorEntity ) {
        return mgfPutCxy(context);
    }
    return MGF_OK;
}

static int
handleIncludedFile(int ac, char **av, RadianceMethod *context)
{
    char *xfarg[MGF_MAXIMUM_ARGUMENT_COUNT];
    MgfReaderContext ictx{};
    MgfTransformContext *xf_orig = GLOBAL_mgf_xfContext;

    if ( ac < 2 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }

    int rv = mgfOpen(&ictx, av[1]);
    if ( rv != MGF_OK ) {
        return rv;
    }
    if ( ac > 2 ) {
        xfarg[0] = GLOBAL_mgf_entityNames[MGF_ENTITY_XF];
        for ( int i = 1; i < ac - 1; i++ ) {
            xfarg[i] = av[i + 1];
        }
        xfarg[ac - 1] = nullptr;
        rv = mgfHandle(MGF_ENTITY_XF, ac - 1, xfarg, context);
        if ( rv != MGF_OK ) {
            mgfClose();
            return rv;
        }
    }
    do {
        while ( (rv = mgfReadNextLine()) > 0 ) {
            if ( rv >= MGF_MAXIMUM_INPUT_LINE_LENGTH - 1 ) {
                fprintf(stderr, "%s: %d: %s\n", ictx.fileName,
                        ictx.lineNumber, GLOBAL_mgf_errors[MGF_ERROR_LINE_TOO_LONG]);
                mgfClose();
                return MGF_ERROR_IN_INCLUDED_FILE;
            }
            rv = mgfParseCurrentLine(context);
            if ( rv != MGF_OK ) {
                fprintf(stderr, "%s: %d: %s:\n%s", ictx.fileName,
                        ictx.lineNumber, GLOBAL_mgf_errors[rv],
                        ictx.inputLine);
                mgfClose();
                return MGF_ERROR_IN_INCLUDED_FILE;
            }
        }
        if ( ac > 2 ) {
            rv = mgfHandle(MGF_ENTITY_XF, 1, xfarg, context);
            if ( rv != MGF_OK ) {
                mgfClose();
                return rv;
            }
        }
    } while ( GLOBAL_mgf_xfContext != xf_orig );
    mgfClose();
    return MGF_OK;
}

/**
rayCasterInitialize alternate entity handlers
*/
static void
mgfAlternativeInit(int (*handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int, char **, RadianceMethod *)) {
    unsigned long ineed = 0;
    unsigned long uneed = 0;
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
        ineed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX;
    } else {
        uneed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_CYLINDER] == nullptr) {
        handleCallbacks[MGF_ENTITY_CYLINDER] = mgfEntityCylinder;
        ineed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX;
    } else {
        uneed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_CONE] == nullptr) {
        handleCallbacks[MGF_ENTITY_CONE] = mgfEntityCone;
        ineed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX;
    } else {
        uneed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_RING] == nullptr) {
        handleCallbacks[MGF_ENTITY_RING] = mgfEntityRing;
        ineed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_NORMAL | 1L << MGF_ENTITY_VERTEX;
    } else {
        uneed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_NORMAL | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_PRISM] == nullptr) {
        handleCallbacks[MGF_ENTITY_PRISM] = mgfEntityPrism;
        ineed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX;
    } else {
        uneed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_TORUS] == nullptr) {
        handleCallbacks[MGF_ENTITY_TORUS] = mgfEntityTorus;
        ineed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_NORMAL | 1L << MGF_ENTITY_VERTEX;
    } else {
        uneed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_NORMAL | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_FACE] == nullptr) {
        handleCallbacks[MGF_ENTITY_FACE] = handleCallbacks[MGF_ENTITY_FACE_WITH_HOLES];
    } else if ( handleCallbacks[MGF_ENTITY_FACE_WITH_HOLES] == nullptr) {
        handleCallbacks[MGF_ENTITY_FACE_WITH_HOLES] = mgfEntityFaceWithHoles;
    }
    if ( handleCallbacks[MGF_ENTITY_COLOR] != nullptr) {
        if ( handleCallbacks[MGF_ENTITY_C_MIX] == nullptr) {
            handleCallbacks[MGF_ENTITY_C_MIX] = mgfECmix;
            ineed |= 1L << MGF_ENTITY_COLOR | 1L << MGF_ENTITY_CXY | 1L << MGF_ENTITY_C_SPEC | 1L << MGF_ENTITY_C_MIX | 1L << MGF_ENTITY_CCT;
        }
        if ( handleCallbacks[MGF_ENTITY_C_SPEC] == nullptr) {
            handleCallbacks[MGF_ENTITY_C_SPEC] = mgfECSpec;
            ineed |= 1L << MGF_ENTITY_COLOR | 1L << MGF_ENTITY_CXY | 1L << MGF_ENTITY_C_SPEC | 1L << MGF_ENTITY_C_MIX | 1L << MGF_ENTITY_CCT;
        }
        if ( handleCallbacks[MGF_ENTITY_CCT] == nullptr) {
            handleCallbacks[MGF_ENTITY_CCT] = mgfColorTemperature;
            ineed |= 1L << MGF_ENTITY_COLOR | 1L << MGF_ENTITY_CXY | 1L << MGF_ENTITY_C_SPEC | 1L << MGF_ENTITY_C_MIX | 1L << MGF_ENTITY_CCT;
        }
    }

    // Check for consistency
    if ( handleCallbacks[MGF_ENTITY_FACE] != nullptr) {
        uneed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_CXY] != nullptr || handleCallbacks[MGF_ENTITY_C_SPEC] != nullptr ||
         handleCallbacks[MGF_ENTITY_C_MIX] != nullptr) {
        uneed |= 1L << MGF_ENTITY_COLOR;
    }
    if ( handleCallbacks[MGF_ENTITY_RD] != nullptr || handleCallbacks[MGF_ENTITY_TD] != nullptr ||
         handleCallbacks[MGF_ENTITY_IR] != nullptr ||
         handleCallbacks[MGF_ENTITY_ED] != nullptr ||
         handleCallbacks[MGF_ENTITY_RS] != nullptr ||
         handleCallbacks[MGF_ENTITY_TS] != nullptr ||
         handleCallbacks[MGF_ENTITY_SIDES] != nullptr) {
        uneed |= 1L << MGF_ENTITY_MATERIAL;
    }
    for ( i = 0; i < MGF_TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( uneed & 1L << i && handleCallbacks[i] == nullptr) {
            fprintf(stderr, "Missing support for \"%s\" entity\n",
                    GLOBAL_mgf_entityNames[i]);
            exit(1);
        }
    }

    // Add support as needed
    if ( ineed & 1L << MGF_ENTITY_VERTEX && handleCallbacks[MGF_ENTITY_VERTEX] != handleVertexEntity ) {
        GLOBAL_mgf_support[MGF_ENTITY_VERTEX] = handleVertexEntity;
    }
    if ( ineed & 1L << MGF_ENTITY_POINT && handleCallbacks[MGF_ENTITY_POINT] != handleVertexEntity ) {
        GLOBAL_mgf_support[MGF_ENTITY_POINT] = handleVertexEntity;
    }
    if ( ineed & 1L << MGF_ENTITY_NORMAL && handleCallbacks[MGF_ENTITY_NORMAL] != handleVertexEntity ) {
        GLOBAL_mgf_support[MGF_ENTITY_NORMAL] = handleVertexEntity;
    }
    if ( ineed & 1L << MGF_ENTITY_COLOR && handleCallbacks[MGF_ENTITY_COLOR] != handleColorEntity ) {
        GLOBAL_mgf_support[MGF_ENTITY_COLOR] = handleColorEntity;
    }
    if ( ineed & 1L << MGF_ENTITY_CXY && handleCallbacks[MGF_ENTITY_CXY] != handleColorEntity ) {
        GLOBAL_mgf_support[MGF_ENTITY_CXY] = handleColorEntity;
    }
    if ( ineed & 1L << MGF_ENTITY_C_SPEC && handleCallbacks[MGF_ENTITY_C_SPEC] != handleColorEntity ) {
        GLOBAL_mgf_support[MGF_ENTITY_C_SPEC] = handleColorEntity;
    }
    if ( ineed & 1L << MGF_ENTITY_C_MIX && handleCallbacks[MGF_ENTITY_C_MIX] != handleColorEntity ) {
        GLOBAL_mgf_support[MGF_ENTITY_C_MIX] = handleColorEntity;
    }
    if ( ineed & 1L << MGF_ENTITY_CCT && handleCallbacks[MGF_ENTITY_CCT] != handleColorEntity ) {
        GLOBAL_mgf_support[MGF_ENTITY_CCT] = handleColorEntity;
    }

    // Discard remaining entities
    for ( i = 0; i < MGF_TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( handleCallbacks[i] == nullptr) {
            handleCallbacks[i] = mgfDiscardUnNeededEntity;
        }
    }
}

static void
initMgf() {
    // Related to MgfColorContext
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_COLOR] = handleColorEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_CXY] = handleColorEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_C_MIX] = handleColorEntity;

    // Related to MgfMaterialContext
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_MATERIAL] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_ED] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_IR] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_RD] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_RS] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_SIDES] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_TD] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_TS] = handleMaterialEntity;

    // Related to MgfTransformContext
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_XF] = handleTransformationEntity;

    // Related to object, no explicit context
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_OBJECT] = handleObjectEntity;

    // Related to geometry elements, no explicit context
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_FACE] = handleFaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_FACE_WITH_HOLES] = handleFaceWithHolesEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_VERTEX] = handleVertexEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_POINT] = handleVertexEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_NORMAL] = handleVertexEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_SPHERE] = handleSurfaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_TORUS] = handleSurfaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_RING] = handleSurfaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_CYLINDER] = handleSurfaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_CONE] = handleSurfaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_PRISM] = handleSurfaceEntity;

    // Default behavior: skip
    GLOBAL_mgf_unknownEntityHandleCallback = handleUnknownEntity;

    mgfAlternativeInit(GLOBAL_mgf_handleCallbacks);
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
readMgf(
    char *filename,
    RadianceMethod *context,
    bool singleSided)
{
    mgfSetNrQuartCircDivs(GLOBAL_fileOptions_numberOfQuarterCircleDivisions);
    mgfSetIgnoreSingleSide(singleSided);
    mgfSetMonochrome(GLOBAL_fileOptions_monochrome);

    initMgf();

    globalPointsOctree = nullptr;
    globalNormalsOctree = nullptr;
    GLOBAL_mgf_currentGeometryList = new java::ArrayList<Geometry *>();

    if ( GLOBAL_scene_materials == nullptr ) {
        GLOBAL_scene_materials = new java::ArrayList<Material *>();
    }
    GLOBAL_mgf_currentMaterial = &GLOBAL_material_defaultMaterial;

    GLOBAL_mgf_geometryStackPtr = GLOBAL_mgf_geometryStack;

    GLOBAL_mgf_inComplex = false;
    GLOBAL_mgf_inSurface = false;

    newSurface();

    MgfReaderContext mgfReaderContext{};
    int status;
    if ( filename[0] == '#' ) {
        status = mgfOpen(&mgfReaderContext, nullptr);
    } else {
        status = mgfOpen(&mgfReaderContext, filename);
    }
    if ( status ) {
        doError(GLOBAL_mgf_errors[status]);
    } else {
        while ( mgfReadNextLine() > 0 && !status ) {
            status = mgfParseCurrentLine(context);
            if ( status ) {
                doError(GLOBAL_mgf_errors[status]);
            }
        }
        mgfClose();
    }
    mgfClear();

    if ( GLOBAL_mgf_inSurface ) {
        surfaceDone();
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
