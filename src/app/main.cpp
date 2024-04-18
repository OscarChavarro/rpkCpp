#include "common/options.h"
#include "material/statistics.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "io/mgf/readmgf.h"
#include "scene/Scene.h"
#include "render/render.h"
#include "render/opengl.h"
#include "render/ScreenBuffer.h"
//#include "render/glutDebugTools.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "app/Cluster.h"
#include "app/radiance.h"
#include "app/commandLine.h"
#include "app/sceneBuilder.h"
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
    ColorRgb outlineColor = DEFAULT_OUTLINE_COLOR;
    ColorRgb boundingBoxColor = DEFAULT_BOUNDING_BOX_COLOR;
    ColorRgb clusterColor = DEFAULT_CLUSTER_COLOR;

    renderUseDisplayLists(DEFAULT_DISPLAY_LISTS);
    renderSetSmoothShading(DEFAULT_SMOOTH_SHADING);
    renderSetBackfaceCulling(DEFAULT_BACKFACE_CULLING);
    renderSetOutlineDrawing(DEFAULT_OUTLINE_DRAWING);
    renderSetBoundingBoxDrawing(DEFAULT_BOUNDING_BOX_DRAWING);
    renderSetClusterDrawing(DEFAULT_CLUSTER_DRAWING);
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
mainInit(Scene *scene) {
    // Transforms the cubature rules for quadrilaterals to be over the domain [0,1]^2 instead of [-1,1]^2
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
mainParseGlobalOptions(int *argc, char **argv, RadianceMethod **context, Camera *camera) {
    renderParseOptions(argc, argv);
    parseToneMapOptions(argc, argv);
    parseCameraOptions(argc, argv, camera, globalImageOutputWidth, globalImageOutputHeight);
    parseRadianceOptions(argc, argv, context);

    #ifdef RAYTRACING_ENABLED
        mainParseRayTracingOptions(argc, argv);
    #endif

    parseBatchOptions(argc, argv);
    parseGeneralProgramOptions(argc, argv, &globalOneSidedSurfaces, &globalConicSubDivisions);
}

static void
createOffscreenCanvasWindow(
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
            scene->camera,
            scene->patchList,
            scene->geometryList,
            scene->clusteredGeometryList,
            scene->clusteredRootGeometry,
            f,
            context);
    #endif
}

static void
mainExecuteRendering(
    Scene *scene,
    int outputImageWidth,
    int outputImageHeight,
    RadianceMethod *radianceMethod)
{
    // Create the window in which to render (canvas window)
    createOffscreenCanvasWindow(
        outputImageWidth,
        outputImageHeight,
        scene,
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
}

int
main(int argc, char *argv[]) {
    RadianceMethod *selectedRadianceMethod = nullptr;
    char *currentDirectory = nullptr;
    Scene scene;

    mainInit(&scene);
    mainParseGlobalOptions(&argc, argv, &selectedRadianceMethod, scene.camera);

    MgfContext mgfContext;
    mgfContext.radianceMethod = selectedRadianceMethod;
    mgfContext.singleSided = globalOneSidedSurfaces;
    mgfContext.numberOfQuarterCircleDivisions = globalConicSubDivisions;
    mgfContext.monochrome = DEFAULT_MONOCHROME;
    mgfContext.currentMaterial = &GLOBAL_material_defaultMaterial;

    mainBuildModel(&argc, argv, &mgfContext, &scene, &currentDirectory);

    mainExecuteRendering(
        &scene,
        globalImageOutputWidth,
        globalImageOutputHeight,
        selectedRadianceMethod);

    //executeGlutGui(argc, argv, &scene, mgfContext.radianceMethod);

    mainFreeMemory(&mgfContext);

    if ( selectedRadianceMethod != nullptr ) {
        delete selectedRadianceMethod;
    }

    if ( currentDirectory != nullptr ) {
        delete[] currentDirectory;
    }

    return 0;
}
