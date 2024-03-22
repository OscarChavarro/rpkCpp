#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "scene/scene.h"
#include "io/mgf/parser.h"
#include "io/mgf/vectoroctree.h"
#include "io/mgf/fileopts.h"
#include "io/mgf/mgfHandlerGeometry.h"
#include "io/mgf/lookup.h"
#include "io/mgf/mgfHandlerTransform.h"
#include "io/mgf/mgfHandlerObject.h"
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
