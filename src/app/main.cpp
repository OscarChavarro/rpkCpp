#include <ctime>
#include <cstring>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/options.h"
#include "common/cubature.h"
#include "material/statistics.h"
#include "render/render.h"
#include "render/renderhook.h"
#include "scene/scene.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "io/mgf/readmgf.h"
#include "render/opengl.h"
#include "render/ScreenBuffer.h"
#include "GALERKIN/GalerkinRadianceMethod.h"
#include "app/Cluster.h"
#include "app/radiance.h"
#include "app/batch.h"
#include "render/glutDebugTools.h"

#ifdef RAYTRACING_ENABLED
    #include "app/raytrace.h"
#endif

// Mgf defaults
#define DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS 4
#define DEFAULT_FORCE_ONE_SIDED true
#define DEFAULT_MONOCHROME false

// Default rendering options
#define DEFAULT_DISPLAY_LISTS false
#define DEFAULT_SMOOTH_SHADING true
#define DEFAULT_BACKFACE_CULLING true
#define DEFAULT_OUTLINE_DRAWING false
#define DEFAULT_BOUNDING_BOX_DRAWING false
#define DEFAULT_CLUSTER_DRAWING false
#define DEFAULT_OUTLINE_COLOR {0.5, 0.0, 0.0}
#define DEFAULT_BOUNDING_BOX_COLOR {0.5, 0.0, 1.0}
#define DEFAULT_CLUSTER_COLOR {1.0, 0.5, 0.0}

extern java::ArrayList<Patch *> *GLOBAL_scenePatches;

// The light of all patches on light sources, useful for e.g. next event estimation in Monte Carlo raytracing etc.
java::ArrayList<Patch *> *GLOBAL_app_lightSourcePatches = nullptr;

// The list of all patches in the current scene. Automatically derived from 'GLOBAL_scene_world' when loading a scene
static java::ArrayList<Patch *> *globalAppScenePatches = nullptr;

static char *globalCurrentDirectory;
static int globalNumberOfQuarterCircleDivisions = DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
static int globalYes = 1;
static int globalNo = 0;
static int globalImageOutputWidth = 0;
static int globalImageOutputHeight = 0;
static int globalFileOptionsForceOneSidedSurfaces = 0;

static void
mainForceOneSidedOption(void *value) {
    globalFileOptionsForceOneSidedSurfaces = *((int *) value);
}

static void
mainMonochromeOption(void *value) {
    globalNumberOfQuarterCircleDivisions = *(int *)value;
}

static CommandLineOptionDescription globalOptions[] = {
    {"-nqcdivs", 3, &GLOBAL_options_intType, &globalNumberOfQuarterCircleDivisions, DEFAULT_ACTION,
    "-nqcdivs <integer>\t: number of quarter circle divisions"},
    {"-force-onesided", 10, TYPELESS, &globalYes, mainForceOneSidedOption,
    "-force-onesided\t\t: force one-sided surfaces"},
    {"-dont-force-onesided", 14, TYPELESS, &globalNo, mainForceOneSidedOption,
    "-dont-force-onesided\t: allow two-sided surfaces"},
    {"-monochromatic", 5, TYPELESS, &globalYes, mainMonochromeOption,
    "-monochromatic \t\t: convert colors to shades of grey"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

static void
mainPatchAccumulateStats(Patch *patch) {
    ColorRgb E = patch->averageEmittance(ALL_COMPONENTS);
    ColorRgb R = patch->averageNormalAlbedo(BSDF_ALL_COMPONENTS);
    ColorRgb power;

    GLOBAL_statistics.totalArea += patch->area;
    power.scaledCopy(patch->area, E);
    GLOBAL_statistics.totalEmittedPower.add(GLOBAL_statistics.totalEmittedPower, power);
    GLOBAL_statistics.averageReflectivity.addScaled(GLOBAL_statistics.averageReflectivity, patch->area, R);
    // Convert radiant exitance to exitant radiance
    E.scale(1.0f / (float) M_PI);
    GLOBAL_statistics.maxSelfEmittedRadiance.maximum(E, GLOBAL_statistics.maxSelfEmittedRadiance);
    GLOBAL_statistics.maxSelfEmittedPower.maximum(power, GLOBAL_statistics.maxSelfEmittedPower);
}

static void
mainComputeSomeSceneStats() {
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
    for ( int i = 0; i < globalAppScenePatches->size(); i++ ) {
        mainPatchAccumulateStats(globalAppScenePatches->get(i));
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
    BP = backgroundPower(GLOBAL_scene_background, &zero);
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
mainAddBackgroundToLightSourceList() {
    if ( GLOBAL_scene_background != nullptr && GLOBAL_scene_background->bkgPatch != nullptr ) {
        GLOBAL_app_lightSourcePatches->add(0, GLOBAL_scene_background->bkgPatch);
    }
}

/**
Adds the patch to the global light source patch list if the patch is on 
a light source (i.e. when the surfaces material has a non-null edf)
*/
static void
mainAddPatchToLightSourceListIfLightSource(Patch *patch) {
    if ( patch != nullptr
         && patch->surface != nullptr
         && patch->surface->material != nullptr
         && patch->surface->material->edf != nullptr ) {
        GLOBAL_app_lightSourcePatches->add(0, patch);
        GLOBAL_statistics.numberOfLightSources++;
    }
}

/**
Build the global light source patch list
*/
static void
mainBuildLightSourcePatchList() {
    GLOBAL_app_lightSourcePatches = new java::ArrayList<Patch *>();
    GLOBAL_statistics.numberOfLightSources = 0;

    for ( int i = 0; i < globalAppScenePatches->size(); i++ ) {
        mainAddPatchToLightSourceListIfLightSource(globalAppScenePatches->get(i));
    }

    mainAddBackgroundToLightSourceList();
    GLOBAL_statistics.numberOfLightSources++;
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

    GLOBAL_render_renderOptions.drawCameras = false;
    GLOBAL_render_renderOptions.cameraSize = 0.25;
    GLOBAL_render_renderOptions.lineWidth = 1.0;
    GLOBAL_render_renderOptions.camera_color = GLOBAL_material_yellow;
    GLOBAL_render_renderOptions.renderRayTracedImage = false;
}

/**
Global initializations
*/
static void
mainInit() {
    // Transforms the cubature rules for quadrilaterals to be over the domain [0,1]^2 instead of [-1,1]^2
    fixCubatureRules();

    globalFileOptionsForceOneSidedSurfaces = DEFAULT_FORCE_ONE_SIDED;
    globalNumberOfQuarterCircleDivisions = DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;

    mainRenderingDefaults();
    toneMapDefaults();
    cameraDefaults();
    radianceDefaults(GLOBAL_scenePatches, nullptr);

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
Processes command line arguments not recognized by the Xt GUI toolkit
*/
static void
mainParseGlobalOptions(int *argc, char **argv, RadianceMethod **context) {
    GLOBAL_scenePatches = globalAppScenePatches;

    renderParseOptions(argc, argv);
    parseToneMapOptions(argc, argv);
    parseCameraOptions(argc, argv);
    parseRadianceOptions(argc, argv, context);

    #ifdef RAYTRACING_ENABLED
        mainParseRayTracingOptions(argc, argv);
    #endif

    parseBatchOptions(argc, argv);
    parseGeneralOptions(globalOptions, argc, argv); // Order is important, this should be called last
}

/**
Builds a linear list of patches making up all the geometries in the list, whether
they are primitive or not
*/
static void
buildPatchList(java::ArrayList<Geometry *> *geometryList, java::ArrayList<Patch *> *patchList) {
    if ( geometryList == nullptr ) {
        return;
    }

    for ( int i = 0; i < geometryList->size(); i++ ) {
        Geometry *geometry = geometryList->get(i);
        if ( geometry->isCompound() ) {
            // Recursive case
            Compound *compound = (Compound *)geometry->compoundData;
            buildPatchList(compound->children, patchList);
        } else {
            // Trivial case
            java::ArrayList<Patch *> *list2 = geomPatchArrayListReference(geometry);

            for ( int j = 0; list2 != nullptr && j < list2->size(); j++ ) {
                Patch *patch = list2->get(j);
                if ( patch != nullptr ) {
                    patchList->add(0, patch);
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

    // Terminate any active radiance or raytracing methods
    fprintf(stderr, "Terminating current radiance/raytracing method ... \n");
    setRadianceMethod(nullptr, GLOBAL_scenePatches);

    #ifdef RAYTRACING_ENABLED
        Raytracer *currentRaytracer = GLOBAL_raytracer_activeRaytracer;
        mainSetRayTracingMethod(nullptr);
    #endif

    // Prepare if errors occur when reading the new scene will abort
    GLOBAL_scene_geometries = nullptr;

    if ( globalAppScenePatches != nullptr ) {
        delete globalAppScenePatches;
    }
    globalAppScenePatches = nullptr;
    Patch::setNextId(1);
    GLOBAL_scene_clusteredGeometries = new java::ArrayList<Geometry *>();
    GLOBAL_scene_background = nullptr;

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
        GLOBAL_scene_geometries = context->geometries;
    }

    clock_t t = clock();
    fprintf(stderr, "Reading took %g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    delete[] globalCurrentDirectory;

    // Check for errors
    if ( GLOBAL_scene_geometries == nullptr || GLOBAL_scene_geometries->size() == 0 ) {
        return false; // Not successful
    }

    // Dispose of the old scene
    fprintf(stderr, "Disposing of the old scene ... ");
    fflush(stderr);
    if ( context ) {
        context->radianceMethod->terminate(GLOBAL_scenePatches);
    }

    #ifdef RAYTRACING_ENABLED
        if ( GLOBAL_raytracer_activeRaytracer != nullptr ) {
            GLOBAL_raytracer_activeRaytracer->Terminate();
        }
    #endif

    delete globalAppScenePatches;

    if ( GLOBAL_scene_background != nullptr ) {
        GLOBAL_scene_background->methods->Destroy(GLOBAL_scene_background->data);
        GLOBAL_scene_background = nullptr;
    }

    if ( GLOBAL_scene_worldVoxelGrid != nullptr ) {
        delete GLOBAL_scene_worldVoxelGrid;
        GLOBAL_scene_worldVoxelGrid = nullptr;
    }

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Build the new patch list, this is duplicating already available
    // information and as such potentially dangerous, but we need it
    // so many times
    fprintf(stderr, "Building patch list ... ");
    fflush(stderr);

    globalAppScenePatches = new java::ArrayList<Patch *>();
    buildPatchList(GLOBAL_scene_geometries, globalAppScenePatches);

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Destilate the list of patches on light sources from the above patch list
    fprintf(stderr, "Building light source patch list ... ");
    fflush(stderr);

    mainBuildLightSourcePatchList();

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Build a cluster hierarchy for the new scene
    fprintf(stderr, "Building cluster hierarchy ... ");
    fflush(stderr);

    GLOBAL_scene_clusteredWorldGeom = mainCreateClusterHierarchy(globalAppScenePatches);

    if ( GLOBAL_scene_clusteredWorldGeom->className == GeometryClassId::COMPOUND ) {
        if ( GLOBAL_scene_clusteredWorldGeom->compoundData == nullptr ) {
            fprintf(stderr, "Unexpected case: review code - generic case not supported anymore.\n");
            exit(2);
        }
    } else {
        logWarning(nullptr, "Strange clusters for this world ...");
    }

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Engridding the thing
    GLOBAL_scene_worldVoxelGrid = new VoxelGrid(GLOBAL_scene_clusteredWorldGeom);

    t = clock();
    fprintf(stderr, "Engridding took %g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Estimate average radiance, for radiance to display RGB conversion
    fprintf(stderr, "Computing some scene statistics ... ");
    fflush(stderr);

    GLOBAL_statistics.numberOfPatches = GLOBAL_statistics.numberOfElements;
    mainComputeSomeSceneStats();
    GLOBAL_statistics.referenceLuminance = 5.42 * ((1.0 - GLOBAL_statistics.averageReflectivity.gray()) *
            GLOBAL_statistics.estimatedAverageRadiance.luminance());

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Initialize tone mapping
    fprintf(stderr, "Initializing tone mapping ... ");
    fflush(stderr);

    initToneMapping(globalAppScenePatches);

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

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
    if ( context != nullptr ) {
        fprintf(stderr, "Initializing radiance computations ... ");
        fflush(stderr);

        setRadianceMethod(context->radianceMethod, globalAppScenePatches);

        t = clock();
        fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
        last = t;
    }

    #ifdef RAYTRACING_ENABLED
        if ( currentRaytracer != nullptr ) {
            fprintf(stderr, "Initializing raytracing computations ... \n");

            mainSetRayTracingMethod(currentRaytracer);

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
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Geometry *> *sceneGeometries,
    RadianceMethod *context)
{
    openGlMesaRenderCreateOffscreenWindow(width, height);

    // Set correct width and height for the camera
    cameraSet(&GLOBAL_camera_mainCamera, &GLOBAL_camera_mainCamera.eyePosition, &GLOBAL_camera_mainCamera.lookPosition,
              &GLOBAL_camera_mainCamera.upDirection,
              GLOBAL_camera_mainCamera.fov, width, height, &GLOBAL_camera_mainCamera.background);

    #ifdef RAYTRACING_ENABLED
        // Render the scene (no expose events on the external canvas window!)
        int (*f)() = nullptr;
        if ( GLOBAL_raytracer_activeRaytracer != nullptr ) {
            f = GLOBAL_raytracer_activeRaytracer->Redisplay;
        }
        openGlRenderScene(scenePatches, sceneGeometries, GLOBAL_scene_clusteredGeometries, f, context);
    #endif
}

static void
mainExecuteRendering(java::ArrayList<Patch *> *scenePatches, RadianceMethod *context) {
    // Create the window in which to render (canvas window)
    if ( globalImageOutputWidth <= 0 ) {
        globalImageOutputWidth = 1920;
    }
    if ( globalImageOutputHeight <= 0 ) {
        globalImageOutputHeight = 1080;
    }
    createOffscreenCanvasWindow(
        globalImageOutputWidth,
        globalImageOutputHeight,
        scenePatches,
        GLOBAL_scene_geometries,
        context);

    while ( !openGlRenderInitialized() ) {}

    #ifdef RAYTRACING_ENABLED
        int (*f)() = nullptr;
        if ( GLOBAL_raytracer_activeRaytracer != nullptr ) {
            f = GLOBAL_raytracer_activeRaytracer->Redisplay;
        }
        openGlRenderScene(
            scenePatches,
            GLOBAL_scene_clusteredGeometries,
            GLOBAL_scene_geometries,
            f,
            context);
    #endif

    batch(scenePatches, GLOBAL_app_lightSourcePatches, GLOBAL_scene_geometries, context);
}

static void
mainFreeMemory(MgfContext *context) {
    deleteOptionsMemory();
    mgfFreeMemory(context);
    galerkinFreeMemory();
    delete GLOBAL_app_lightSourcePatches;
    delete GLOBAL_scene_clusteredGeometries;
    if ( globalAppScenePatches != nullptr ) {
        delete globalAppScenePatches;
    }
    if ( GLOBAL_scene_worldVoxelGrid != nullptr ) {
        delete GLOBAL_scene_worldVoxelGrid;
        GLOBAL_scene_worldVoxelGrid = nullptr;
    }
}

int
main(int argc, char *argv[]) {
    RadianceMethod *selectedRadianceMethod = nullptr;

    mainInit();
    mainParseGlobalOptions(&argc, argv, &selectedRadianceMethod);

    MgfContext mgfContext;
    mgfContext.radianceMethod = selectedRadianceMethod;
    mgfContext.singleSided = globalFileOptionsForceOneSidedSurfaces != 0;
    mgfContext.numberOfQuarterCircleDivisions = globalNumberOfQuarterCircleDivisions;
    mgfContext.monochrome = DEFAULT_MONOCHROME;
    mgfContext.currentMaterial = &GLOBAL_material_defaultMaterial;

    mainBuildModel(&argc, argv, &mgfContext);

    mainExecuteRendering(globalAppScenePatches, selectedRadianceMethod);

    //executeGlutGui(argc, argv, globalAppScenePatches, GLOBAL_app_lightSourcePatches, GLOBAL_scene_geometries, mgfContext.radianceMethod);

    mainFreeMemory(&mgfContext);

    return 0;
}
