#include "common/options.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "io/mgf/readmgf.h"
#include "render/render.h"
#include "render/opengl.h"
//#include "render/glutDebugTools.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "GALERKIN/processing/ClusterCreationStrategy.h"
#include "scene/Cluster.h"
#include "app/commandLine.h"
#include "app/sceneBuilder.h"
#include "app/radiance.h"
#include "app/batch.h"
#include "app/RpkApplication.h"

#ifdef RAYTRACING_ENABLED
    #include "app/raytrace.h"
#endif

#define DEFAULT_MONOCHROME false

RpkApplication::RpkApplication() {
    scene = new Scene();
}

RpkApplication::~RpkApplication() {
    delete scene;
}

/**
Global initializations
*/
static void
mainInitApplication(Scene *scene) {
    fixCubatureRules();
    toneMapDefaults();
    radianceDefaults(nullptr, scene);

#ifdef RAYTRACING_ENABLED
    mainRayTracingDefaults();
#endif

    // Default vertex compare flags: both location and normal is relevant. Two
    // vertices without normal, but at the same location, are to be considered
    // different
    Vertex::setCompareFlags(VERTEX_COMPARE_LOCATION | VERTEX_COMPARE_NORMAL);
}

/**
Processes command line arguments
*/
static void
mainParseOptions(
    int *argc,
    char **argv,
    RadianceMethod **radianceMethod,
    Camera *camera,
    MgfContext *mgfContext,
    int *outputImageWidth,
    int *outputImageHeight,
    RenderOptions *renderOptions)
{
    commandLineGeneralProgramParseOptions(
        argc,
        argv,
        &mgfContext->singleSided,
        &mgfContext->numberOfQuarterCircleDivisions,
        outputImageWidth,
        outputImageHeight);
    renderParseOptions(argc, argv, renderOptions);
    toneMapParseOptions(argc, argv);
    cameraParseOptions(argc, argv, camera, *outputImageWidth, *outputImageHeight);
    radianceParseOptions(argc, argv, radianceMethod);

#ifdef RAYTRACING_ENABLED
    rayTracingParseOptions(argc, argv);
#endif

    batchParseOptions(argc, argv);
}

static void
mainCreateOffscreenCanvasWindow(
    const int outputImageWidth,
    const int outputImageHeight,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions)
{
    openGlMesaRenderCreateOffscreenWindow(scene->camera, outputImageWidth, outputImageHeight);

    // Set correct outputImageWidth and outputImageHeight for the camera
    scene->camera->xSize = outputImageWidth;
    scene->camera->ySize = outputImageHeight;

    #ifdef RAYTRACING_ENABLED
        // Render the scene
        int (*renderCallback)() = nullptr;
        if ( GLOBAL_raytracer_activeRaytracer != nullptr ) {
            renderCallback = GLOBAL_raytracer_activeRaytracer->Redisplay;
        }
        openGlRenderScene(scene, renderCallback, radianceMethod, renderOptions);
    #endif
}

static void
mainExecuteRendering(
    int outputImageWidth,
    int outputImageHeight,
    Scene *scene,
    RadianceMethod *radianceMethod,
    RenderOptions *renderOptions)
{
    // Create the window in which to render (canvas window)
    mainCreateOffscreenCanvasWindow(outputImageWidth, outputImageHeight, scene, radianceMethod, renderOptions);

    #ifdef RAYTRACING_ENABLED
        int (*renderCallback)() = nullptr;
        if ( GLOBAL_raytracer_activeRaytracer != nullptr ) {
            renderCallback = GLOBAL_raytracer_activeRaytracer->Redisplay;
        }
        openGlRenderScene(scene, renderCallback, radianceMethod, renderOptions);
    #endif

    batchExecuteRadianceSimulation(scene, radianceMethod, renderOptions);
}

static void
mainFreeMemory(MgfContext *mgfContext) {
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
    mainInitApplication(scene);

    // 2. Set model elements from command line options
    int imageOutputWidth;
    int imageOutputHeight;
    MgfContext mgfContext;
    RadianceMethod *selectedRadianceMethod = nullptr;
    RenderOptions renderOptions;

    mainParseOptions(
        &argc,
        argv,
        &selectedRadianceMethod,
        scene->camera,
        &mgfContext,
        &imageOutputWidth,
        &imageOutputHeight,
        &renderOptions);

    // 3. Load scene elements from MGF file
    Material defaultMaterial("(default)", nullptr, nullptr, false);
    mgfContext.radianceMethod = selectedRadianceMethod;
    mgfContext.monochrome = DEFAULT_MONOCHROME;
    mgfContext.currentMaterial = &defaultMaterial;
    sceneBuilderCreateModel(&argc, argv, &mgfContext, scene);

    // 4. Run main radiosity simulation and export result
    mainExecuteRendering(imageOutputWidth, imageOutputHeight, scene, selectedRadianceMethod, &renderOptions);

    // X. Interactive visual debug GUI tool
    //executeGlutGui(argc, argv, scene, mgfContext.radianceMethod, &renderOptions, mainFreeMemory, &mgfContext);

    // 5. Free used memory
    mainFreeMemory(&mgfContext);

    return 0;
}
