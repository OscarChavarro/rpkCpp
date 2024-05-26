#include "common/numericalAnalysis/QuadCubatureRule.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "io/mgf/readmgf.h"
#include "render/opengl.h"
//#include "render/glutDebugTools.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "GALERKIN/processing/ClusterCreationStrategy.h"
#include "scene/Cluster.h"
#include "app/options.h"
#include "app/commandLine.h"
#include "app/sceneBuilder.h"
#include "app/radiance.h"
#include "app/batch.h"
#include "app/RpkApplication.h"

#ifdef RAYTRACING_ENABLED
    #include "raycasting/render/RayTracingRenderer.h"
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
        delete rayTracer;
    }
}

/**
Global initializations
*/
void
RpkApplication::mainInitApplication() {
    QuadCubatureRule::fixCubatureRules();
    toneMapDefaults();
    radianceDefaults(nullptr, scene);

    // Default vertex compare flags: both location and normal is relevant. Two
    // vertices without normal, but at the same location, are to be considered
    // different
    Vertex::setCompareFlags(VERTEX_COMPARE_LOCATION | VERTEX_COMPARE_NORMAL);
}

/**
Processes command line arguments
*/
void
RpkApplication::mainParseOptions(int *argc, char **argv, char *rayTracerName) {
    commandLineGeneralProgramParseOptions(
        argc,
        argv,
        &mgfContext->singleSided,
        &mgfContext->numberOfQuarterCircleDivisions,
        &imageOutputWidth,
        &imageOutputHeight);
    renderParseOptions(argc, argv, renderOptions);
    toneMapParseOptions(argc, argv);
    cameraParseOptions(argc, argv, scene->camera, imageOutputWidth, imageOutputHeight);
    radianceParseOptions(argc, argv, &selectedRadianceMethod);

#ifdef RAYTRACING_ENABLED
    rayTraceParseOptions(argc, argv, rayTracerName);
#endif

    generalParseOptions(argc, argv);
}

void
RpkApplication::mainCreateOffscreenCanvasWindow() {
    openGlMesaRenderCreateOffscreenWindow(scene->camera, imageOutputWidth, imageOutputHeight);

    // Set correct outputImageWidth and outputImageHeight for the camera
    scene->camera->xSize = imageOutputWidth;
    scene->camera->ySize = imageOutputHeight;

    #ifdef RAYTRACING_ENABLED
        // Render the scene
        if ( GLOBAL_rayTracer != nullptr ) {
            openGlRenderScene(scene, GLOBAL_rayTracer, selectedRadianceMethod, renderOptions);
        }
    #endif
}

void
RpkApplication::executeRendering(const char *rayTracerName) {
    // Create the window in which to render (canvas window)
    mainCreateOffscreenCanvasWindow();

    #ifdef RAYTRACING_ENABLED
        rayTracer = rayTraceCreate(scene, rayTracerName);
        GLOBAL_rayTracer = rayTracer;

        if ( GLOBAL_rayTracer != nullptr ) {
            openGlRenderScene(scene, GLOBAL_rayTracer, selectedRadianceMethod, renderOptions);
        }
    #endif

    batchExecuteRadianceSimulation(scene, selectedRadianceMethod, rayTracer, renderOptions);
}

void
RpkApplication::freeMemory(MgfContext *mgfContext) {
    deleteOptionsMemory();
    mgfFreeMemory(mgfContext);
    galerkinFreeMemory();
    Cluster::deleteCachedGeometries();
    ClusterCreationStrategy::freeClusterElements();
    VoxelGrid::freeVoxelGridElements();
    if ( mgfContext->radianceMethod != nullptr ) {
        delete mgfContext->radianceMethod;
    }
}

int
RpkApplication::entryPoint(int argc, char *argv[]) {
    // 1. Default empty scene
    mainInitApplication();

    // 2. Set model elements from command line options
    char rayTracerName[256];
    mainParseOptions(&argc, argv, rayTracerName);

    // 3. Load scene elements from MGF file
    mgfContext->radianceMethod = selectedRadianceMethod;
    mgfContext->monochrome = DEFAULT_MONOCHROME;
    mgfContext->currentMaterial = &defaultMaterial;
    sceneBuilderCreateModel(&argc, argv, mgfContext, scene);

    // 4. Run main radiosity simulation and export result
    executeRendering(rayTracerName);

    // X. Interactive visual debug GUI tool
    //executeGlutGui(argc, argv, scene, mgfContext->radianceMethod, renderOptions, RpkApplication::freeMemory, mgfContext);

    // 5. Free used memory
    freeMemory(mgfContext);

    return 0;
}
