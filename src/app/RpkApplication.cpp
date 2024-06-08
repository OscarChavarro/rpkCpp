#include <cstring>
#include "common/numericalAnalysis/QuadCubatureRule.h"
#include "tonemap/ToneMap.h"
#include "tonemap/LightnessToneMap.h"
#include "tonemap/RevisedTumblinRushmeierToneMap.h"
#include "tonemap/TumblinRushmeierToneMap.h"
#include "tonemap/WardToneMap.h"
#include "tonemap/FerwerdaToneMap.h"
#include "io/mgf/readmgf.h"
#include "io/image/dkcolor.h"
#include "render/glutDebugTools.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "GALERKIN/processing/ClusterCreationStrategy.h"
#include "scene/PatchClusterOctreeNode.h"
#include "app/options.h"
#include "app/commandLine.h"
#include "app/sceneBuilder.h"
#include "app/radiance.h"
#include "app/batch.h"
#include "app/RpkApplication.h"

#ifdef RAYTRACING_ENABLED
#include "raycasting/bidirectionalRaytracing/LightList.h"
    #include "app/raytrace.h"
#endif

static const bool DEFAULT_MONOCHROME = false;

Material RpkApplication::defaultMaterial("(default)", nullptr, nullptr, false);

RpkApplication::RpkApplication():
    imageOutputWidth(),
    imageOutputHeight(),
    selectedRadianceMethod(),
    rayTracer()
{
    scene = new Scene();
    mgfContext = new MgfContext();
    renderOptions = new RenderOptions();
}

RpkApplication::~RpkApplication() {
    delete scene;
    delete mgfContext;
    delete renderOptions;
    if ( rayTracer != nullptr ) {
        rayTracer->terminate();
        delete rayTracer;
    }
}

/**
Global initializations
*/
void
RpkApplication::mainInitApplication() {
    QuadCubatureRule::fixCubatureRules();

    // Default vertex compare flags: both location and normal is relevant. Two
    // vertices without normal, but at the same location, are to be considered
    // different
    Vertex::setCompareFlags(VERTEX_COMPARE_LOCATION | VERTEX_COMPARE_NORMAL);
}

void
RpkApplication::selectToneMapByName(const char *name) {
    ToneMap *newMap;

    if ( strcmp(name, "TumblinRushmeier") == 0 ) {
        newMap = new TumblinRushmeierToneMap();
    } else if ( strcmp(name, "Ward") == 0 ) {
        newMap = new WardToneMap();
    } else if ( strcmp(name, "RevisedTR") == 0 ) {
        newMap = new RevisedTumblinRushmeierToneMap();
    } else if ( strcmp(name, "Ferwerda") == 0 ) {
        newMap = new FerwerdaToneMap();
    } else {
        newMap = new LightnessToneMap();
    }

    GLOBAL_toneMap_options.selectedToneMap = newMap;
    newMap->init();
}

/**
Processes command line arguments
*/
void
RpkApplication::mainParseOptions(int *argc, char **argv, char *rayTracerName, char *toneMapName) {
    commandLineGeneralProgramParseOptions(
        argc,
        argv,
        &mgfContext->singleSided,
        &mgfContext->numberOfQuarterCircleDivisions,
        &imageOutputWidth,
        &imageOutputHeight);
    renderParseOptions(argc, argv, renderOptions);
    toneMapParseOptions(argc, argv, toneMapName);
    cameraParseOptions(argc, argv, scene->camera, imageOutputWidth, imageOutputHeight);
    radianceParseOptions(argc, argv, &selectedRadianceMethod);

#ifdef RAYTRACING_ENABLED
    rayTraceParseOptions(argc, argv, rayTracerName);
#endif

    generalParseOptions(argc, argv);
}

void
RpkApplication::mainCreateOffscreenCanvasWindow() {
    // Set correct outputImageWidth and outputImageHeight for the camera
    scene->camera->xSize = imageOutputWidth;
    scene->camera->ySize = imageOutputHeight;
}

void
RpkApplication::executeRendering(const char *rayTracerName) {
    // Create the window in which to render (canvas window)
    mainCreateOffscreenCanvasWindow();

    #ifdef RAYTRACING_ENABLED
        rayTracer = rayTraceCreate(scene, rayTracerName);
        GLOBAL_rayTracer = rayTracer;
    #endif

    batchExecuteRadianceSimulation(scene, selectedRadianceMethod, rayTracer, renderOptions);
}

void
RpkApplication::freeMemory(MgfContext *mgfContext) {
    deleteOptionsMemory();
    mgfFreeMemory(mgfContext);
    galerkinFreeMemory();
    PatchClusterOctreeNode::deleteCachedGeometries();
    ClusterCreationStrategy::freeClusterElements();
    VoxelGrid::freeVoxelGridElements();
    if ( mgfContext->radianceMethod != nullptr ) {
        delete mgfContext->radianceMethod;
    }
    dkColorFreeBuffer();
#ifdef RAYTRACING_ENABLED
    if ( GLOBAL_lightList != nullptr ) {
        delete GLOBAL_lightList;
    }
#endif
}

int
RpkApplication::entryPoint(int argc, char *argv[]) {
    // 1. Default empty scene
    mainInitApplication();

    // 2. Set model elements from command line options
    char rayTracerName[256];
    char initializationToneMapName[256] = "Lightness";
    char renderToneMapName[256] = "Lightness";
    mainParseOptions(&argc, argv, rayTracerName, renderToneMapName);

    // 3. Load scene elements from MGF file
    mgfContext->radianceMethod = selectedRadianceMethod;
    mgfContext->monochrome = DEFAULT_MONOCHROME;
    mgfContext->currentMaterial = &defaultMaterial;
    selectToneMapByName(initializationToneMapName); // Note this is used for basic Galerkin model initialization
    sceneBuilderCreateModel(&argc, argv, mgfContext, scene);
    selectToneMapByName(renderToneMapName);

    // 4. Run main radiosity simulation and export result
    executeRendering(rayTracerName);

    // X. Interactive visual debug GUI tool
    executeGlutGui(argc, argv, scene, mgfContext->radianceMethod, renderOptions, RpkApplication::freeMemory, mgfContext);

    // 5. Free used memory
    freeMemory(mgfContext);

    return 0;
}
