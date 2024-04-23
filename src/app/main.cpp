#include "common/options.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "scene/Scene.h"
#include "io/mgf/readmgf.h"
#include "render/render.h"
#include "render/opengl.h"
#include "render/glutDebugTools.h"
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

static int globalImageOutputWidth = 1920;
static int globalImageOutputHeight = 1080;
static bool globalOneSidedSurfaces;
static int globalConicSubDivisions;

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
mainParseOptions(int *argc, char **argv, RadianceMethod **context, Camera *camera) {
    renderParseOptions(argc, argv);
    toneMapParseOptions(argc, argv);
    cameraParseOptions(argc, argv, camera, globalImageOutputWidth, globalImageOutputHeight);
    radianceParseOptions(argc, argv, context);

#ifdef RAYTRACING_ENABLED
    rayTracingParseOptions(argc, argv);
#endif

    batchParseOptions(argc, argv);
    commandLineGeneralProgramParseOptions(argc, argv, &globalOneSidedSurfaces, &globalConicSubDivisions);
}

static void
mainCreateOffscreenCanvasWindow(
    int width,
    int height,
    Camera *camera,
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Geometry *> *sceneGeometries,
    java::ArrayList<Geometry *> *sceneClusteredGeometries,
    Geometry *clusteredWorldGeometry,
    RadianceMethod *context)
{
    openGlMesaRenderCreateOffscreenWindow(camera, width, height);

    // Set correct width and height for the camera
    camera->set(
            &camera->eyePosition,
            &camera->lookPosition,
            &camera->upDirection,
            camera->fieldOfVision,
            width,
            height,
            &camera->background);

    #ifdef RAYTRACING_ENABLED
        // Render the scene (no expose events on the external canvas window!)
        int (*f)() = nullptr;
        if ( GLOBAL_raytracer_activeRaytracer != nullptr ) {
            f = GLOBAL_raytracer_activeRaytracer->Redisplay;
        }
        openGlRenderScene(
            camera,
            scenePatches,
            sceneGeometries,
            sceneClusteredGeometries,
            clusteredWorldGeometry,
            f,
            context);
    #endif
}

static void
mainExecuteRendering(int outputImageWidth, int outputImageHeight, Scene *scene, RadianceMethod *radianceMethod) {
    // Create the window in which to render (canvas window)
    mainCreateOffscreenCanvasWindow(
        outputImageWidth,
        outputImageHeight,
        scene->camera,
        scene->patchList,
        scene->geometryList,
        scene->clusteredGeometryList,
        scene->clusteredRootGeometry,
        radianceMethod);

    #ifdef RAYTRACING_ENABLED
        int (*f)() = nullptr;
        if ( GLOBAL_raytracer_activeRaytracer != nullptr ) {
            f = GLOBAL_raytracer_activeRaytracer->Redisplay;
        }
        openGlRenderScene(
            scene->camera,
            scene->patchList,
            scene->clusteredGeometryList,
            scene->geometryList,
            scene->clusteredRootGeometry,
            f,
            radianceMethod);
    #endif

    batch(
        scene->camera,
        scene->background,
        scene->patchList,
        scene->lightSourcePatchList,
        scene->geometryList,
        scene->clusteredGeometryList,
        scene->clusteredRootGeometry,
        scene->voxelGrid,
        radianceMethod);
}

static void
mainFreeMemory(MgfContext *context) {
    deleteOptionsMemory();
    mgfFreeMemory(context);
    galerkinFreeMemory();
    Cluster::deleteCachedGeometries();
    freeClusterGalerkinElements();
    VoxelGrid::freeVoxelGridElements();
}

int
main(int argc, char *argv[]) {
    Scene scene;
    mainInitApplication(&scene);

    RadianceMethod *selectedRadianceMethod = nullptr;
    mainParseOptions(&argc, argv, &selectedRadianceMethod, scene.camera);

    Material defaultMaterial;
    MgfContext mgfContext;
    mgfContext.radianceMethod = selectedRadianceMethod;
    mgfContext.singleSided = globalOneSidedSurfaces;
    mgfContext.numberOfQuarterCircleDivisions = globalConicSubDivisions;
    mgfContext.monochrome = DEFAULT_MONOCHROME;
    mgfContext.currentMaterial = &defaultMaterial;

    sceneBuilderCreateModel(&argc, argv, &mgfContext, &scene);

    mainExecuteRendering(globalImageOutputWidth, globalImageOutputHeight, &scene, selectedRadianceMethod);

    executeGlutGui(argc, argv, &scene, mgfContext.radianceMethod);

    mainFreeMemory(&mgfContext);

    if ( selectedRadianceMethod != nullptr ) {
        delete selectedRadianceMethod;
    }

    return 0;
}
