#include "common/options.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "scene/Scene.h"
#include "io/mgf/readmgf.h"
#include "render/render.h"
#include "render/opengl.h"
//#include "render/glutDebugTools.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "GALERKIN/clustergalerkincpp.h"
#include "scene/Cluster.h"
#include "app/commandLine.h"
#include "app/sceneBuilder.h"
#include "app/radiance.h"
#include "app/batch.h"

#ifdef RAYTRACING_ENABLED
    #include "app/raytrace.h"
#endif

#define DEFAULT_MONOCHROME false
#define DEFAULT_DISPLAY_LISTS false
#define DEFAULT_SMOOTH_SHADING true
#define DEFAULT_BACKFACE_CULLING true
#define DEFAULT_OUTLINE_DRAWING false
#define DEFAULT_BOUNDING_BOX_DRAWING false
#define DEFAULT_CLUSTER_DRAWING false
#define DEFAULT_OUTLINE_COLOR {0.5, 0.0, 0.0}
#define DEFAULT_BOUNDING_BOX_COLOR {0.5, 0.0, 1.0}
#define DEFAULT_CLUSTER_COLOR {1.0, 0.5, 0.0}

static void
mainRenderingDefaults() {
    renderUseDisplayLists(DEFAULT_DISPLAY_LISTS);
    renderSetSmoothShading(DEFAULT_SMOOTH_SHADING);
    renderSetBackfaceCulling(DEFAULT_BACKFACE_CULLING);
    renderSetOutlineDrawing(DEFAULT_OUTLINE_DRAWING);
    renderSetBoundingBoxDrawing(DEFAULT_BOUNDING_BOX_DRAWING);
    renderSetClusterDrawing(DEFAULT_CLUSTER_DRAWING);

    ColorRgb outlineColor = DEFAULT_OUTLINE_COLOR;
    ColorRgb boundingBoxColor = DEFAULT_BOUNDING_BOX_COLOR;
    ColorRgb clusterColor = DEFAULT_CLUSTER_COLOR;

    renderSetOutlineColor(&outlineColor);
    renderSetBoundingBoxColor(&boundingBoxColor);
    renderSetClusterColor(&clusterColor);
    renderUseFrustumCulling(true);
    renderSetNoShading(false);

    GLOBAL_render_renderOptions.lineWidth = 1.0;
    GLOBAL_render_renderOptions.renderRayTracedImage = false;
}

/**
Global initializations
*/
static void
mainInitApplication(Scene *scene) {
    fixCubatureRules();
    mainRenderingDefaults();
    toneMapDefaults();
    radianceDefaults(nullptr, scene->camera, scene->clusteredRootGeometry);

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
    RadianceMethod **context,
    Camera *camera,
    bool *oneSidedSurfaces,
    int *conicSubdivisions,
    int imageOutputWidth,
    int imageOutputHeight)
{
    renderParseOptions(argc, argv);
    toneMapParseOptions(argc, argv);
    cameraParseOptions(argc, argv, camera, imageOutputWidth, imageOutputHeight);
    radianceParseOptions(argc, argv, context);

#ifdef RAYTRACING_ENABLED
    rayTracingParseOptions(argc, argv);
#endif

    batchParseOptions(argc, argv);
    commandLineGeneralProgramParseOptions(argc, argv, oneSidedSurfaces, conicSubdivisions);
}

static void
mainCreateOffscreenCanvasWindow(
    int width,
    int height,
    Scene *scene,
    RadianceMethod *context)
{
    openGlMesaRenderCreateOffscreenWindow(scene->camera, width, height);

    // Set correct width and height for the camera
    scene->camera->set(
        &scene->camera->eyePosition,
        &scene->camera->lookPosition,
        &scene->camera->upDirection,
        scene->camera->fieldOfVision,
        width,
        height,
        &scene->camera->background);

    #ifdef RAYTRACING_ENABLED
        // Render the scene (no expose events on the external canvas window!)
        int (*f)() = nullptr;
        if ( GLOBAL_raytracer_activeRaytracer != nullptr ) {
            f = GLOBAL_raytracer_activeRaytracer->Redisplay;
        }
        openGlRenderScene(
            scene,
            f,
            context);
    #endif
}

static void
mainExecuteRendering(int outputImageWidth, int outputImageHeight, Scene *scene, RadianceMethod *radianceMethod) {
    // Create the window in which to render (canvas window)
    mainCreateOffscreenCanvasWindow(outputImageWidth, outputImageHeight, scene, radianceMethod);

    #ifdef RAYTRACING_ENABLED
        int (*f)() = nullptr;
        if ( GLOBAL_raytracer_activeRaytracer != nullptr ) {
            f = GLOBAL_raytracer_activeRaytracer->Redisplay;
        }
        openGlRenderScene(scene, f, radianceMethod);
    #endif

    batchExecuteRadianceSimulation(scene, radianceMethod);
}

static void
mainFreeMemory(MgfContext *context) {
    deleteOptionsMemory();
    mgfFreeMemory(context);
    galerkinFreeMemory();
    Cluster::deleteCachedGeometries();
    freeClusterGalerkinElements();
    VoxelGrid::freeVoxelGridElements();
    if ( context->radianceMethod != nullptr ) {
        delete context->radianceMethod;
    }
}

int
main(int argc, char *argv[]) {
    Scene scene;
    mainInitApplication(&scene);

    int imageOutputWidth = 1920;
    int imageOutputHeight = 1080;
    MgfContext mgfContext;
    RadianceMethod *selectedRadianceMethod = nullptr;
    mainParseOptions(
        &argc,
        argv,
        &selectedRadianceMethod,
        scene.camera,
        &mgfContext.singleSided,
        &mgfContext.numberOfQuarterCircleDivisions,
        imageOutputWidth,
        imageOutputHeight);

    Material defaultMaterial;
    mgfContext.radianceMethod = selectedRadianceMethod;
    mgfContext.monochrome = DEFAULT_MONOCHROME;
    mgfContext.currentMaterial = &defaultMaterial;

    sceneBuilderCreateModel(&argc, argv, &mgfContext, &scene);
    mainExecuteRendering(imageOutputWidth, imageOutputHeight, &scene, selectedRadianceMethod);

    //executeGlutGui(argc, argv, &scene, mgfContext.radianceMethod);

    mainFreeMemory(&mgfContext);

    return 0;
}
