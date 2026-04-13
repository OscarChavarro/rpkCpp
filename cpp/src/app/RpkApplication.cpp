#include <cstring>

#include "java/lang/System.h"
#include "common/RenderOptions.h"
#include "scene/PatchClusterOctreeNode.h"
#include "tonemap/FerwerdaToneMap.h"
#include "tonemap/LightnessToneMap.h"
#include "tonemap/RevisedTumblinRushmeierToneMap.h"
#include "tonemap/ToneMap.h"
#include "tonemap/TumblinRushmeierToneMap.h"
#include "tonemap/WardToneMap.h"
#include "io/image/Dkcolor.h"
#include "galerkin/GalerkinRadianceMethod.h"
#include "galerkin/processing/ClusterCreationStrategy.h"
#include "app/Batch.h"
#include "app/options/OptionsGroupCore.h"
#include "app/Radiance.h"
#include "app/RpkApplication.h"
#include "app/SceneBuilder.h"

#ifdef RAYTRACING_ENABLED
    #include "raycasting/simple/RayMatterState.h"
    #include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
    #include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
    #include "raycasting/stochasticRaytracing/StochasticRelaxation.h"
    #include "raycasting/stochasticRaytracing/ElementHierarchyState.h"
    #include "raycasting/stochasticRaytracing/Basismcrad.h"
    #include "raycasting/photonMap/PhotonMapState.h"
    #include "raycasting/photonMap/PhotonMapConfig.h"
#endif

#ifdef OPEN_GL_ENABLED
    #include "render/opengl/visualDebugTools/GlutDebugTools.h"
    #include "render/opengl/visualDebugTools/GlutDebugToolsModel.h"
    #include "render/opengl/visualDebugTools/GlutDebugState.h"
#endif

#ifdef MGF_ENABLED
    #include "io/mgf/MgfParserLoader.h"
#endif

#ifdef RAYTRACING_ENABLED
    #include "raycasting/bidirectionalRaytracing/LightList.h"
    #include "app/Raytrace.h"
#endif

Material RpkApplication::defaultMaterial("(default)", nullptr, nullptr, false);

namespace {
#ifndef RAYTRACING_ENABLED
bool
isRaytracingDependentOption(const char *argument) {
    if ( argument == nullptr ) {
        return false;
    }

    return strcmp(argument, "-raytracing-method") == 0
           || strncmp(argument, "-rts-", 5) == 0
           || strncmp(argument, "-bidir-", 7) == 0
           || strncmp(argument, "-rm-", 4) == 0
           || strncmp(argument, "-srr-", 5) == 0
           || strncmp(argument, "-rwr-", 5) == 0
           || strncmp(argument, "-pmap-", 6) == 0;
}

void
failIfUnsupportedRaytracingOptionRequested(int argc, char **argv) {
    for ( int i = 0; i < argc; i++ ) {
        if ( isRaytracingDependentOption(argv[i]) ) {
            java::System::err.printf(
                "ERROR: Option '%s' requires raytracing support. Rebuild with CMake flag '-DWITH_RAYTRACING=ON'.\n",
                argv[i]);
            java::System::err.flush();
            java::System::exit(1);
        }
    }
}
#endif
}

RpkApplication::RpkApplication():
    imageOutputWidth(),
    imageOutputHeight(),
    selectedRadianceMethod(),
    selectedToneMap(nullptr),
    toneMapOptions(),
    rayTracer(),
    glutDebugEnabled(false)
{
    scene = new Scene();
    mgfContext = new ParseRuntimeContext();
    renderOptions = new RenderOptions();
}

RpkApplication::~RpkApplication() {
    if ( selectedToneMap != nullptr ) {
        delete selectedToneMap;
        selectedToneMap = nullptr;
    }
    ToneMap::setActiveToneMap(nullptr);
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

    if ( selectedToneMap != nullptr ) {
        delete selectedToneMap;
    }
    selectedToneMap = newMap;
    ToneMap::setActiveToneMap(selectedToneMap);
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
        char *toneMapName
#ifdef RAYTRACING_ENABLED
        ,
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState,
        StochasticRadiosityBasisState &stochasticRadiosityBasisState,
        PhotonMapState &photonMapState,
        PhotonMapConfig &photonMapConfig,
        RayMatterState &rayMatterState,
        BidirectionalPathTracingState &bidirectionalPathState,
        StochasticRayTracingState &stochasticRayTracingState
#endif
        )
{
#ifndef RAYTRACING_ENABLED
    failIfUnsupportedRaytracingOptionRequested(*argc, argv);
    (void) rayTracerName;
#endif

    OptionsGroupCore::parse(
        argc,
        argv,
        *mgfContext,
        *scene,
        *renderOptions,
        toneMapOptions,
        imageOutputWidth,
        imageOutputHeight,
        glutDebugEnabled,
        toneMapName);
    Radiance::radianceParseOptions(
        argc,
        argv,
        &selectedRadianceMethod
#ifdef RAYTRACING_ENABLED
        ,
        stochasticRelaxationState,
        elementHierarchyState,
        stochasticRadiosityBasisState,
        photonMapState,
        photonMapConfig,
        rayMatterState,
        bidirectionalPathState,
        stochasticRayTracingState
#endif
        );

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
    const char *rayTracerName
#ifdef RAYTRACING_ENABLED
    ,
    RayMatterState &rayMatterState,
    BidirectionalPathTracingState &bidirectionalPathState,
    StochasticRayTracingState &stochasticRayTracingState,
    LightList *&lightList
#endif
    )
{
    // Create the window in which to render (canvas window)
    mainCreateOffscreenCanvasWindow();

    #ifdef RAYTRACING_ENABLED
        rayTracer = Raytrace::rayTraceCreate(
            scene,
            &toneMapOptions,
            rayTracerName,
            rayMatterState,
            bidirectionalPathState,
            stochasticRayTracingState,
            lightList);
    #else
        (void) rayTracerName;
    #endif

    Batch::batchExecuteRadianceSimulation(scene, selectedRadianceMethod, rayTracer, &toneMapOptions, renderOptions);
}

void
RpkApplication::freeMemory(ParseRuntimeContext *mgfContext) {
#ifdef MGF_ENABLED
    MgfParserLoader::mgfFreeMemory(mgfContext);
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

#ifdef RAYTRACING_ENABLED
    RayMatterState rayMatterState;
    BidirectionalPathTracingState bidirectionalPathState;
    StochasticRayTracingState stochasticRayTracingState;
    StochasticRelaxation stochasticRelaxationState;
    ElementHierarchyState elementHierarchyState;
    StochasticRadiosityBasisState stochasticRadiosityBasisState;
    PhotonMapState photonMapState;
    PhotonMapConfig photonMapConfig;
    LightList *lightList = nullptr;
    StochasticRelaxation::setActiveState(stochasticRelaxationState);
    ElementHierarchyState::setActiveState(elementHierarchyState);
    StochasticRadiosityBasisState::setActiveState(stochasticRadiosityBasisState);
#endif

    // 2. Set model elements from command line options
    char rayTracerName[256] = "none";
    char initializationToneMapName[256] = "Lightness";
    char renderToneMapName[256] = "Lightness";
    mainParseOptions(
        &argc,
        argv,
        rayTracerName,
        renderToneMapName
#ifdef RAYTRACING_ENABLED
        ,
        stochasticRelaxationState,
        elementHierarchyState,
        stochasticRadiosityBasisState,
        photonMapState,
        photonMapConfig,
        rayMatterState,
        bidirectionalPathState,
        stochasticRayTracingState
#endif
        );

    // 3. Load scene elements from MGF file
    mgfContext->radianceMethod = selectedRadianceMethod;
    mgfContext->monochrome = DEFAULT_MONOCHROME;
    mgfContext->currentMaterial = &defaultMaterial;
    selectToneMapByName(initializationToneMapName); // Note this is used for basic Galerkin model initialization
    SceneBuilder::sceneBuilderCreateModel(&argc, argv, mgfContext, scene, toneMapOptions);
    selectToneMapByName(renderToneMapName);

    // 4. Run main radiosity simulation and export result
    executeRendering(
        rayTracerName
#ifdef RAYTRACING_ENABLED
        ,
        rayMatterState,
        bidirectionalPathState,
        stochasticRayTracingState,
        lightList
#endif
        );

    // X. Interactive visual debug GUI tool
    #ifdef OPEN_GL_ENABLED
        if ( glutDebugEnabled ) {
            GlutDebugState debugState;
            GlutDebugToolsModel debugToolsModel;
            debugToolsModel.scene = scene;
            debugToolsModel.radianceMethod = mgfContext->radianceMethod;
            debugToolsModel.renderOptions = renderOptions;
            debugToolsModel.toneMapOptions = &toneMapOptions;
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
    }
    if ( lightList != nullptr ) {
        delete lightList;
        lightList = nullptr;
    }
#endif

    return 0;
}
