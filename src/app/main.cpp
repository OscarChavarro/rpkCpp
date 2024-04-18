#include <ctime>
#include <cstring>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/options.h"
#include "material/statistics.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "io/mgf/readmgf.h"
#include "render/render.h"
#include "render/renderhook.h"
#include "render/opengl.h"
#include "render/ScreenBuffer.h"
//#include "render/glutDebugTools.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "app/Cluster.h"
#include "app/radiance.h"
#include "app/commandLine.h"
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

// The light of all patches on light sources, useful for e.g. next event estimation in Monte Carlo raytracing etc.
static java::ArrayList<Patch *> *globalLightSourcePatches = nullptr;

// The list of all patches in the current scene
static java::ArrayList<Patch *> *globalScenePatches = nullptr;

static char *globalCurrentDirectory;
static int globalImageOutputWidth = 1920;
static int globalImageOutputHeight = 1080;
static bool globalOneSidedSurfaces;
static int globalConicSubDivisions;

static Background *globalSceneBackground = nullptr;
static java::ArrayList<Geometry *> *globalSceneGeometries = nullptr;
static java::ArrayList<Geometry *> *globalSceneClusteredGeometries = nullptr;
static Geometry *globalSceneClusteredWorldGeometry = nullptr;
static VoxelGrid *globalSceneWorldVoxelGrid = nullptr;

static void
mainPatchAccumulateStats(Patch *patch) {
    ColorRgb E = patch->averageEmittance(ALL_COMPONENTS);
    ColorRgb R = patch->averageNormalAlbedo(BSDF_ALL_COMPONENTS);
    ColorRgb power;

    GLOBAL_statistics.totalArea += patch->area;
    power.scaledCopy(patch->area, E);
    GLOBAL_statistics.totalEmittedPower.add(GLOBAL_statistics.totalEmittedPower, power);
    GLOBAL_statistics.averageReflectivity.addScaled(GLOBAL_statistics.averageReflectivity, patch->area, R);
    E.scale(1.0f / (float) M_PI);
    GLOBAL_statistics.maxSelfEmittedRadiance.maximum(E, GLOBAL_statistics.maxSelfEmittedRadiance);
    GLOBAL_statistics.maxSelfEmittedPower.maximum(power, GLOBAL_statistics.maxSelfEmittedPower);
}

static void
mainComputeSomeSceneStats(java::ArrayList<Patch *> *scenePatches) {
    Vector3D zero;
    ColorRgb one;
    ColorRgb averageAbsorption;
    ColorRgb BP;

    one.setMonochrome(1.0f);
    zero.set(0, 0, 0);

    // Initialize
    GLOBAL_statistics.totalEmittedPower.clear();
    GLOBAL_statistics.averageReflectivity.clear();
    GLOBAL_statistics.maxSelfEmittedRadiance.clear();
    GLOBAL_statistics.maxSelfEmittedPower.clear();
    GLOBAL_statistics.totalArea = 0.0;

    // Accumulate
    for ( int i = 0; i < scenePatches->size(); i++ ) {
        mainPatchAccumulateStats(scenePatches->get(i));
    }

    // Averages
    GLOBAL_statistics.averageReflectivity.scaleInverse(
        GLOBAL_statistics.totalArea,
        GLOBAL_statistics.averageReflectivity);
    averageAbsorption.subtract(one, GLOBAL_statistics.averageReflectivity);
    GLOBAL_statistics.estimatedAverageRadiance.scaleInverse(
        M_PI * GLOBAL_statistics.totalArea,
        GLOBAL_statistics.totalEmittedPower);

    // Include background radiation
    BP = backgroundPower(globalSceneBackground, &zero);
    BP.scale(1.0 / (4.0 * (double) M_PI));
    GLOBAL_statistics.totalEmittedPower.add(GLOBAL_statistics.totalEmittedPower, BP);
    GLOBAL_statistics.estimatedAverageRadiance.add(GLOBAL_statistics.estimatedAverageRadiance, BP);
    GLOBAL_statistics.estimatedAverageRadiance.divide(GLOBAL_statistics.estimatedAverageRadiance, averageAbsorption);

    GLOBAL_statistics.totalDirectPotential = 0.0;
    GLOBAL_statistics.maxDirectPotential = 0.0;
    GLOBAL_statistics.averageDirectPotential = 0.0;
    GLOBAL_statistics.maxDirectImportance = 0.0;
}

/**
Adds the background to the global light source patch list
*/
static void
mainAddBackgroundToLightSourceList(java::ArrayList<Patch *> *lights) {
    if ( globalSceneBackground != nullptr && globalSceneBackground->bkgPatch != nullptr ) {
        lights->add(globalSceneBackground->bkgPatch);
        GLOBAL_statistics.numberOfLightSources++;
    }
}

/**
Adds the patch to the global light source patch list if the patch is on 
a light source (i.e. when the surfaces material has a non-null edf)
*/
static void
mainAddPatchToLightSourceListIfLightSource(java::ArrayList<Patch *> *lights, Patch *patch) {
    if ( patch != nullptr
         && patch->material != nullptr
         && patch->material->edf != nullptr ) {
        lights->add(patch);
        GLOBAL_statistics.numberOfLightSources++;
    }
}

/**
Build the global light source patch list
*/
static java::ArrayList<Patch *> *
mainBuildLightSourcePatchList(java::ArrayList<Patch *> *scenePatches) {
    java::ArrayList<Patch *> *lights = new java::ArrayList<Patch *>();
    GLOBAL_statistics.numberOfLightSources = 0;

    for ( int i = 0; i < scenePatches->size(); i++ ) {
        mainAddPatchToLightSourceListIfLightSource(lights, scenePatches->get(i));
    }

    mainAddBackgroundToLightSourceList(lights);
    return lights;
}

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
mainInit() {
    // Transforms the cubature rules for quadrilaterals to be over the domain [0,1]^2 instead of [-1,1]^2
    fixCubatureRules();

    mainRenderingDefaults();
    toneMapDefaults();
    radianceDefaults(nullptr, globalSceneClusteredWorldGeometry);

    #ifdef RAYTRACING_ENABLED
        mainRayTracingDefaults();
    #endif

    // Default vertex compare flags: both location and normal is relevant. Two
    // vertices without normal, but at the same location, are to be considered
    // different
    Vertex::setCompareFlags(VERTEX_COMPARE_LOCATION | VERTEX_COMPARE_NORMAL);
}

/**
Creates a hierarchical model of the discrete scene (the patches in the scene) using the simple
algorithm described in
- Per Christensen, "Hierarchical Techniques for Glossy Global Illumination",
  PhD Thesis, University of Washington, 1995, p 116
This hierarchy is often much more efficient for tracing rays and clustering radiosity algorithms
than the given hierarchy of bounding boxes. A pointer to the toplevel "cluster" is returned
*/
static Geometry *
mainCreateClusterHierarchy(java::ArrayList<Patch *> *patches) {
    Cluster *rootCluster;
    Geometry *rootGeometry;

    // Create a toplevel cluster containing (references to) all the patches in the solid
    rootCluster = new Cluster(patches);

    // Split the toplevel cluster recursively into sub-clusters
    rootCluster->splitCluster();

    // Convert to a Geometry GLOBAL_stochasticRaytracing_hierarchy, disposing of the clusters
    rootGeometry = rootCluster->convertClusterToGeometry();

    delete rootCluster;
    return rootGeometry;
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

/**
Builds a linear list of patches making up all the geometries in the list, whether
they are primitive or not
*/
static void
buildPatchList(java::ArrayList<Geometry *> *geometryList, java::ArrayList<Patch *> *patchList) {
    for ( int i = 0; i < geometryList->size(); i++ ) {
        Geometry *geometry = geometryList->get(i);
        if ( geometry->isCompound() ) {
            // Recursive case
            Compound *compound = (Compound *)geometry->compoundData;
            buildPatchList(compound->children, patchList);
        } else {
            // Trivial case
            java::ArrayList<Patch *> *patchesFromNonCompounds = geomPatchArrayListReference(geometry);

            for ( int j = 0; patchesFromNonCompounds != nullptr && j < patchesFromNonCompounds->size(); j++ ) {
                Patch *patch = patchesFromNonCompounds->get(j);
                if ( patch != nullptr ) {
                    patchList->add(patch);
                }
            }
        }
    }
}

/**
Tries to read the scene in the given file. Returns false if not successful.
Returns true if successful. There's nothing GUI specific in this function.
When a file cannot be read, the current scene is restored
*/
static bool
mainReadFile(char *filename, MgfContext *context) {
    // Check whether the file can be opened if not reading from stdin
    if ( filename[0] != '#' ) {
        FILE *input = fopen(filename, "r");
        if ( input == nullptr || fgetc(input) == EOF ) {
            if ( input != nullptr ) {
                fclose(input);
            }
            logError(nullptr, "Can't open file '%s' for reading", filename);
            return false;
        }
        fclose(input);
    }

    // Get current directory from the filename
    unsigned long n = strlen(filename) + 1;

    globalCurrentDirectory = new char[n];
    snprintf(globalCurrentDirectory, n, "%s", filename);
    char *slash = strrchr(globalCurrentDirectory, '/');
    if ( slash != nullptr ) {
        *slash = '\0';
    } else {
        *globalCurrentDirectory = '\0';
    }

    // Init compute method
    setRadianceMethod(nullptr, nullptr, globalSceneClusteredWorldGeometry);

    #ifdef RAYTRACING_ENABLED
        Raytracer *currentRaytracer = GLOBAL_raytracer_activeRaytracer;
        mainSetRayTracingMethod(nullptr, nullptr);
    #endif

    // Prepare if errors occur when reading the new scene will abort
    globalSceneGeometries = nullptr;

    Patch::setNextId(1);
    globalSceneClusteredGeometries = new java::ArrayList<Geometry *>();
    globalSceneBackground = nullptr;

    // Read the mgf file. The result is a new GLOBAL_scene_world and GLOBAL_scene_materials if everything goes well
    char *extension;
    fprintf(stderr, "Reading the scene from file '%s' ... \n", filename);
    clock_t last = clock();

    char *dot = strrchr(filename, '.');
    if ( dot != nullptr ) {
        extension = dot + 1;
    } else {
        extension = (char *)"mgf";
    }

    if ( strncmp(extension, "mgf", 3) == 0 ) {
        readMgf(filename, context);
        globalSceneGeometries = context->geometries;
    }

    clock_t t = clock();
    fprintf(stderr, "Reading took %g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    delete[] globalCurrentDirectory;

    // Check for errors
    if ( globalSceneGeometries == nullptr || globalSceneGeometries->size() == 0 ) {
        return false; // Not successful
    }

    // Build the new patch list, this is duplicating already available
    // information and as such potentially dangerous, but we need it
    // so many times
    fprintf(stderr, "Building patch list ... ");
    fflush(stderr);

    globalScenePatches = new java::ArrayList<Patch *>();
    buildPatchList(globalSceneGeometries, globalScenePatches);

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Build the list of patches on light sources from the patch list
    fprintf(stderr, "Building light source patch list ... ");
    fflush(stderr);

    globalLightSourcePatches = mainBuildLightSourcePatchList(globalScenePatches);

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Build a cluster hierarchy for the new scene
    fprintf(stderr, "Building cluster hierarchy ... ");
    fflush(stderr);

    globalSceneClusteredWorldGeometry = mainCreateClusterHierarchy(globalScenePatches);

    if ( globalSceneClusteredWorldGeometry->className == GeometryClassId::COMPOUND ) {
        if ( globalSceneClusteredWorldGeometry->compoundData == nullptr ) {
            fprintf(stderr, "Unexpected case: review code - generic case not supported anymore.\n");
            exit(2);
        }
    } else {
        logWarning(nullptr, "Strange clusters for this world ...");
    }

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Create the scene level voxel grid
    globalSceneWorldVoxelGrid = new VoxelGrid(globalSceneClusteredWorldGeometry);

    t = clock();
    fprintf(stderr, "Voxel grid creation took %g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Estimate average radiance, for radiance to display RGB conversion
    fprintf(stderr, "Computing some scene statistics ... ");
    fflush(stderr);

    GLOBAL_statistics.numberOfPatches = GLOBAL_statistics.numberOfElements;
    mainComputeSomeSceneStats(globalScenePatches);
    GLOBAL_statistics.referenceLuminance = 5.42 * ((1.0 - GLOBAL_statistics.averageReflectivity.gray()) *
        GLOBAL_statistics.estimatedAverageRadiance.luminance());

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Initialize tone mapping
    fprintf(stderr, "Initializing tone mapping ... ");
    fflush(stderr);

    initToneMapping(globalScenePatches);

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Print statistics report
    printf("\nStats: GLOBAL_statistics.totalEmittedPower ................: %f W\n"
           "         GLOBAL_statistics.estimatedAverageRadiance .........: %f W / sr\n"
           "         GLOBAL_statistics_averageReflectivity ..............: %f\n"
           "         GLOBAL_statistics.maxSelfEmittedRadiance ...........: %f W / sr\n"
           "         GLOBAL_statistics.maxSelfEmittedPower ..............: %f W\n"
           "         GLOBAL_toneMap_options.lwa (adaptationLuminance) ...: %f cd / m2\n"
           "         GLOBAL_statistics_totalArea ........................: %f m2\n",
           GLOBAL_statistics.totalEmittedPower.gray(),
           GLOBAL_statistics.estimatedAverageRadiance.gray(),
           GLOBAL_statistics.averageReflectivity.gray(),
           GLOBAL_statistics.maxSelfEmittedRadiance.gray(),
           GLOBAL_statistics.maxSelfEmittedPower.gray(),
           GLOBAL_toneMap_options.realWorldAdaptionLuminance,
           GLOBAL_statistics.totalArea);

    // Initialize radiance for the freshly loaded scene
    fprintf(stderr, "Initializing radiance method ... ");
    fflush(stderr);

    setRadianceMethod(context->radianceMethod, globalScenePatches, globalSceneClusteredWorldGeometry);

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    #ifdef RAYTRACING_ENABLED
        if ( currentRaytracer != nullptr ) {
            fprintf(stderr, "Initializing raytracing method ... \n");

            mainSetRayTracingMethod(currentRaytracer, globalLightSourcePatches);

            t = clock();
            fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
        }
    #endif

    // Remove possible render hooks
    removeAllRenderHooks();

    fprintf(stderr, "Initialisations done.\n");

    return true;
}

static void
mainBuildModel(const int *argc, char *const *argv, MgfContext *context) {
    // All options should have disappeared from argv now
    if ( *argc > 1 ) {
        if ( *argv[1] == '-' ) {
            logError(nullptr, "Unrecognized option '%s'", argv[1]);
        } else if ( !mainReadFile(argv[1], context) ) {
            exit(1);
        }
    }
}

static void
createOffscreenCanvasWindow(
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
mainExecuteRendering(
    Camera *camera,
    int outputImageWidth,
    int outputImageHeight,
    Background *background,
    java::ArrayList<Geometry *> *sceneGeometries,
    java::ArrayList<Geometry *> *sceneClusteredGeometries,
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Patch *> *sceneLightPatches,
    Geometry *sceneClusteredWorldGeometry,
    VoxelGrid *sceneVoxelGrid,
    RadianceMethod *radianceMethod)
{
    // Create the window in which to render (canvas window)
    createOffscreenCanvasWindow(
        outputImageWidth,
        outputImageHeight,
        camera,
        scenePatches,
        sceneGeometries,
        sceneClusteredGeometries,
        sceneClusteredWorldGeometry,
        radianceMethod);

    #ifdef RAYTRACING_ENABLED
        int (*f)() = nullptr;
        if ( GLOBAL_raytracer_activeRaytracer != nullptr ) {
            f = GLOBAL_raytracer_activeRaytracer->Redisplay;
        }
        openGlRenderScene(
            camera,
            scenePatches,
            sceneClusteredGeometries,
            sceneGeometries,
            sceneClusteredWorldGeometry,
            f,
            radianceMethod);
    #endif

    batch(
        camera,
        background,
        scenePatches,
        sceneLightPatches,
        sceneGeometries,
        sceneClusteredGeometries,
        sceneClusteredWorldGeometry,
        sceneVoxelGrid,
        radianceMethod);
}

static void
mainFreeMemory(MgfContext *context) {
    deleteOptionsMemory();
    mgfFreeMemory(context);
    galerkinFreeMemory();
    delete globalLightSourcePatches;
    delete globalSceneClusteredGeometries;
    if ( globalScenePatches != nullptr ) {
        delete globalScenePatches;
    }
    if ( globalSceneWorldVoxelGrid != nullptr ) {
        delete globalSceneWorldVoxelGrid;
        globalSceneWorldVoxelGrid = nullptr;
    }
}

int
main(int argc, char *argv[]) {
    RadianceMethod *selectedRadianceMethod = nullptr;

    mainInit();
    mainParseGlobalOptions(&argc, argv, &selectedRadianceMethod, &GLOBAL_camera_mainCamera);

    MgfContext mgfContext;
    mgfContext.radianceMethod = selectedRadianceMethod;
    mgfContext.singleSided = globalOneSidedSurfaces;
    mgfContext.numberOfQuarterCircleDivisions = globalConicSubDivisions;
    mgfContext.monochrome = DEFAULT_MONOCHROME;
    mgfContext.currentMaterial = &GLOBAL_material_defaultMaterial;

    mainBuildModel(&argc, argv, &mgfContext);

    mainExecuteRendering(
        &GLOBAL_camera_mainCamera,
        globalImageOutputWidth,
        globalImageOutputHeight,
        globalSceneBackground,
        globalSceneGeometries,
        globalSceneClusteredGeometries,
        globalScenePatches,
        globalLightSourcePatches,
        globalSceneClusteredWorldGeometry,
        globalSceneWorldVoxelGrid,
        selectedRadianceMethod);

    //executeGlutGui(
    //    argc,
    //    argv,
    //    &GLOBAL_camera_mainCamera,
    //    globalScenePatches,
    //    globalLightSourcePatches,
    //    globalSceneGeometries,
    //    globalSceneClusteredGeometries,
    //    globalSceneBackground,
    //    globalSceneClusteredWorldGeometry,
    //    mgfContext.radianceMethod,
    //    globalSceneWorldVoxelGrid);

    mainFreeMemory(&mgfContext);

    if ( selectedRadianceMethod != nullptr ) {
        delete selectedRadianceMethod;
    }

    return 0;
}
