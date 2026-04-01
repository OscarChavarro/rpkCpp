#include <cstring>

#include "common/RenderOptions.h"
#include "common/statistics/Statistics.h"
#include "scene/PatchClusterOctreeNode.h"
#include "tonemap/FerwerdaToneMap.h"
#include "tonemap/LightnessToneMap.h"
#include "tonemap/RevisedTumblinRushmeierToneMap.h"
#include "tonemap/TumblinRushmeierToneMap.h"
#include "tonemap/WardToneMap.h"
#include "raycasting/simple/RayMatterState.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "io/image/Dkcolor.h"
#include "galerkin/GalerkinRadianceMethod.h"
#include "galerkin/processing/ClusterCreationStrategy.h"
#include "app/Batch.h"
#include "app/CommandLine.h"
#include "app/Options.h"
#include "app/Radiance.h"
#include "app/RpkApplication.h"
#include "app/SceneBuilder.h"

#ifdef OPEN_GL_ENABLED
    #include "render/opengl/visualDebugTools/GlutDebugTools.h"
    #include "render/opengl/visualDebugTools/GlutDebugToolsModel.h"
    #include "render/opengl/visualDebugTools/GlutDebugState.h"
#endif

#ifdef MGF_ENABLED
    #include "io/mgf/MgfReader.h"
#endif

#ifdef RAYTRACING_ENABLED
    #include "raycasting/bidirectionalRaytracing/LightList.h"
    #include "app/Raytrace.h"
#endif

static constexpr bool DEFAULT_MONOCHROME = false;

Material RpkApplication::defaultMaterial("(default)", nullptr, nullptr, false);

RpkApplication::RpkApplication():
    imageOutputWidth(),
    imageOutputHeight(),
    selectedRadianceMethod(),
    toneMapOptions(),
    rayTracer(),
    glutDebugEnabled(false)
{
    scene = new Scene();
    mgfContext = new ParseSession();
    renderOptions = new RenderOptions();
    scene->toneMapOptions = &toneMapOptions;
    renderOptions->toneMapOptions = &toneMapOptions;
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

    if ( toneMapOptions.selectedToneMap != nullptr ) {
        delete toneMapOptions.selectedToneMap;
    }
    toneMapOptions.selectedToneMap = newMap;
    newMap->init(toneMapOptions);
}

/**
Processes command line arguments
*/
void
RpkApplication::mainParseOptions(
    int *argc,
    char **argv,
    char *rayTracerName,
    char *toneMapName,
    RayMatterState &rayMatterState,
    BidirectionalPathTracingState &bidirectionalPathState,
    StochasticRayTracingState &stochasticRayTracingState)
{
    CommandLine::commandLineGeneralProgramParseOptions(
        argc,
        argv,
        &mgfContext->singleSided,
        &mgfContext->numberOfQuarterCircleDivisions,
        &imageOutputWidth,
        &imageOutputHeight,
        &glutDebugEnabled);
    CommandLine::renderParseOptions(argc, argv, renderOptions);
    renderOptions->toneMapOptions = &toneMapOptions;
    CommandLine::toneMapParseOptions(argc, argv, toneMapName, toneMapOptions);
    CommandLine::cameraParseOptions(argc, argv, scene->camera, imageOutputWidth, imageOutputHeight);
    Radiance::radianceParseOptions(
        argc,
        argv,
        &selectedRadianceMethod,
        rayMatterState,
        bidirectionalPathState,
        stochasticRayTracingState);

#ifdef RAYTRACING_ENABLED
    Raytrace::rayTraceParseOptions(argc, argv, rayTracerName);
#endif

    Batch::generalParseOptions(argc, argv);
}

void
RpkApplication::mainCreateOffscreenCanvasWindow() const {
    // Set correct outputImageWidth and outputImageHeight for the camera
    scene->camera->xSize = imageOutputWidth;
    scene->camera->ySize = imageOutputHeight;
}

void
RpkApplication::executeRendering(
    const char *rayTracerName,
    RayMatterState &rayMatterState,
    BidirectionalPathTracingState &bidirectionalPathState,
    StochasticRayTracingState &stochasticRayTracingState,
    LightList *&lightList)
{
    // Create the window in which to render (canvas window)
    mainCreateOffscreenCanvasWindow();

    #ifdef RAYTRACING_ENABLED
        rayTracer = Raytrace::rayTraceCreate(
            scene,
            rayTracerName,
            rayMatterState,
            bidirectionalPathState,
            stochasticRayTracingState,
            lightList);
        Statistics::instance().rayTracer.currentRayTracer = rayTracer;
    #else
        (void) rayTracerName;
        (void) rayMatterState;
        (void) bidirectionalPathState;
        (void) stochasticRayTracingState;
        (void) lightList;
    #endif

    Batch::batchExecuteRadianceSimulation(scene, selectedRadianceMethod, rayTracer, renderOptions);
}

void
RpkApplication::freeMemory(ParseSession *mgfContext) {
    Options::deleteOptionsMemory();
#ifdef MGF_ENABLED
    MgfReader::mgfFreeMemory(mgfContext);
#endif
    GalerkinRadianceMethod::freeMemory();
    PatchClusterOctreeNode::deleteCachedGeometries();
    ClusterCreationStrategy::freeClusterElements();
    VoxelGrid::freeVoxelGridElements();
    if ( mgfContext->radianceMethod != nullptr ) {
        delete mgfContext->radianceMethod;
    }
    DkColor::freeBuffer();
}

int
RpkApplication::entryPoint(int argc, char *argv[]) {
    // 1. Default empty scene
    mainInitApplication();

    RayMatterState rayMatterState;
    BidirectionalPathTracingState bidirectionalPathState;
    StochasticRayTracingState stochasticRayTracingState;
    LightList *lightList = nullptr;

    // 2. Set model elements from command line options
    char rayTracerName[256];
    char initializationToneMapName[256] = "Lightness";
    char renderToneMapName[256] = "Lightness";
    mainParseOptions(
        &argc,
        argv,
        rayTracerName,
        renderToneMapName,
        rayMatterState,
        bidirectionalPathState,
        stochasticRayTracingState);

    // 3. Load scene elements from MGF file
    mgfContext->radianceMethod = selectedRadianceMethod;
    mgfContext->monochrome = DEFAULT_MONOCHROME;
    mgfContext->currentMaterial = &defaultMaterial;
    selectToneMapByName(initializationToneMapName); // Note this is used for basic Galerkin model initialization
    SceneBuilder::sceneBuilderCreateModel(&argc, argv, mgfContext, scene, toneMapOptions);
    selectToneMapByName(renderToneMapName);

    // 4. Run main radiosity simulation and export result
    executeRendering(
        rayTracerName,
        rayMatterState,
        bidirectionalPathState,
        stochasticRayTracingState,
        lightList);

    // X. Interactive visual debug GUI tool
    #ifdef OPEN_GL_ENABLED
        if ( glutDebugEnabled ) {
            GlutDebugState debugState;
            GlutDebugToolsModel debugToolsModel;
            debugToolsModel.scene = scene;
            debugToolsModel.radianceMethod = mgfContext->radianceMethod;
            debugToolsModel.renderOptions = renderOptions;
            debugToolsModel.debugState = &debugState;
            debugToolsModel.memoryFreeCallBack = RpkApplication::freeMemory;
            debugToolsModel.mgfContext = mgfContext;

            GlutDebugTools *glutDebugTools = new GlutDebugTools(debugToolsModel);
            glutDebugTools->executeGlutGui(argc, argv);
            delete glutDebugTools;
        }
    #endif

    // 5. Free used memory
    freeMemory(mgfContext);

#ifdef RAYTRACING_ENABLED
    if ( rayTracer != nullptr ) {
        rayTracer->terminate();
        delete rayTracer;
        rayTracer = nullptr;
        Statistics::instance().rayTracer.currentRayTracer = nullptr;
    }
    if ( lightList != nullptr ) {
        delete lightList;
        lightList = nullptr;
    }
#endif

    return 0;
}
