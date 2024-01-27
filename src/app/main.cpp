#include <ctime>
#include <cstring>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "BREP/BREP_VERTEX_OCTREE.h"
#include "material/statistics.h"
#include "skin/radianceinterfaces.h"
#include "shared/defaults.h"
#include "shared/options.h"
#include "shared/render.h"
#include "shared/cubature.h"
#include "shared/renderhook.h"
#include "scene/scene.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "io/mgf/readmgf.h"
#include "io/mgf/fileopts.h"
#include "app/opengl.h"
#include "app/Cluster.h"
#include "app/batch.h"
#include "app/mainModel.h"
#include "app/mainRendering.h"

static java::ArrayList<Patch *> *
convertPatchSetToPatchList(PatchSet *patchSet) {
    java::ArrayList<Patch *> *newList = new java::ArrayList<Patch *>();

    for ( PatchSet *window = patchSet; window != nullptr; window = window->next ) {
        newList->add(0, window->patch);
    }

    return newList;
}

/**
This routine sets the current raytracing method to be used
*/
static void
mainSetRayTracingMethod(Raytracer *newMethod) {
    if ( GLOBAL_raytracer_activeRaytracer ) {
        GLOBAL_raytracer_activeRaytracer->InterruptRayTracing();
        GLOBAL_raytracer_activeRaytracer->Terminate();
    }

    GLOBAL_raytracer_activeRaytracer = newMethod;
    if ( GLOBAL_raytracer_activeRaytracer ) {
        GLOBAL_raytracer_activeRaytracer->Initialize();
    }
}

static void
mainRayTracingOption(void *value) {
    char *name = *(char **) value;

    for ( Raytracer **methodpp = globalRayTracingMethods; *methodpp; methodpp++ ) {
        Raytracer *method = *methodpp;
        if ( strncasecmp(name, method->shortName, method->nameAbbrev) == 0 ) {
            mainSetRayTracingMethod(method);
            return;
        }
    }

    if ( strncasecmp(name, "none", 4) == 0 ) {
        mainSetRayTracingMethod(nullptr);
    } else {
        logError(nullptr, "Invalid raytracing method name '%s'", name);
    }
}

static CommandLineOptionDescription globalRaytracingOptions[] = {
    {"-raytracing-method", 4, Tstring,  nullptr, mainRayTracingOption, globalRaytracingMethodsString},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

static void
mainForceOneSidedOption(void *value) {
    GLOBAL_fileOptions_forceOneSidedSurfaces = *((int *) value);
}

static void
mainMonochromeOption(void *value) {
    GLOBAL_fileOptions_monochrome = *(int *) value;
}

static CommandLineOptionDescription globalOptions[] = {
    {"-nqcdivs", 3, Tint, &GLOBAL_fileOptions_numberOfQuarterCircleDivisions, DEFAULT_ACTION,
    "-nqcdivs <integer>\t: number of quarter circle divisions"},
    {"-force-onesided", 10, TYPELESS, &globalYes,     mainForceOneSidedOption,
    "-force-onesided\t\t: force one-sided surfaces"},
    {"-dont-force-onesided", 14, TYPELESS, &globalNo, mainForceOneSidedOption,
    "-dont-force-onesided\t: allow two-sided surfaces"},
    {"-monochromatic", 5, TYPELESS, &globalYes, mainMonochromeOption,
    "-monochromatic \t\t: convert colors to shades of grey"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

static void
mainPatchAccumulateStats(Patch *patch) {
    COLOR E = patchAverageEmittance(patch, ALL_COMPONENTS);
    COLOR R = patchAverageNormalAlbedo(patch, BSDF_ALL_COMPONENTS);
    COLOR power;

    GLOBAL_statistics_totalArea += patch->area;
    colorScale(patch->area, E, power);
    colorAdd(GLOBAL_statistics_totalEmittedPower, power, GLOBAL_statistics_totalEmittedPower);
    colorAddScaled(GLOBAL_statistics_averageReflectivity, patch->area, R, GLOBAL_statistics_averageReflectivity);
    // Convert radiant exitance to exitant radiance
    colorScale((1.0 / (float)M_PI), E, E);
    colorMaximum(E, GLOBAL_statistics_maxSelfEmittedRadiance, GLOBAL_statistics_maxSelfEmittedRadiance);
    colorMaximum(power, GLOBAL_statistics_maxSelfEmittedPower, GLOBAL_statistics_maxSelfEmittedPower);
}

static void
mainComputeSomeSceneStats() {
    Vector3D zero;
    COLOR one;
    COLOR average_absorption;
    COLOR BP;

    colorSetMonochrome(one, 1.0f);
    VECTORSET(zero, 0, 0, 0);

    // Initialize
    colorClear(GLOBAL_statistics_totalEmittedPower);
    colorClear(GLOBAL_statistics_averageReflectivity);
    colorClear(GLOBAL_statistics_maxSelfEmittedRadiance);
    colorClear(GLOBAL_statistics_maxSelfEmittedPower);
    GLOBAL_statistics_totalArea = 0.;

    // Accumulate
    java::ArrayList<Patch *> *scenePatches = convertPatchSetToPatchList(globalAppScenePatches);
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        mainPatchAccumulateStats(scenePatches->get(i));
    }

    // Averages
    colorScaleInverse(GLOBAL_statistics_totalArea, GLOBAL_statistics_averageReflectivity, GLOBAL_statistics_averageReflectivity);
    colorSubtract(one, GLOBAL_statistics_averageReflectivity, average_absorption);
    colorScaleInverse(M_PI * GLOBAL_statistics_totalArea, GLOBAL_statistics_totalEmittedPower, GLOBAL_statistics_estimatedAverageRadiance);

    // Include background radiation
    BP = backgroundPower(GLOBAL_scene_background, &zero);
    colorScale(1.0 / (4.0 * (double)M_PI), BP, BP);
    colorAdd(GLOBAL_statistics_totalEmittedPower, BP, GLOBAL_statistics_totalEmittedPower);
    colorAdd(GLOBAL_statistics_estimatedAverageRadiance, BP, GLOBAL_statistics_estimatedAverageRadiance);

    colorDivide(GLOBAL_statistics_estimatedAverageRadiance, average_absorption,
                GLOBAL_statistics_estimatedAverageRadiance);

    GLOBAL_statistics_totalDirectPotential = GLOBAL_statistics_maxDirectPotential = GLOBAL_statistics_averageDirectPotential = GLOBAL_statistics_maxDirectImportance = 0.;
}

/**
Adds the background to the global light source patch list
*/
static void
mainAddBackgroundToLightSourceList() {
    if ( GLOBAL_scene_background != nullptr && GLOBAL_scene_background->bkgPatch != nullptr ) {
        PatchSet *newListNode = (PatchSet *)malloc(sizeof(PatchSet));
        newListNode->patch = GLOBAL_scene_background->bkgPatch;
        newListNode->next = GLOBAL_scene_lightSourcePatches;
        GLOBAL_scene_lightSourcePatches = newListNode;
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
        PatchSet *newListNode = (PatchSet *)malloc(sizeof(PatchSet));
        newListNode->patch = patch;
        newListNode->next = GLOBAL_scene_lightSourcePatches;
        GLOBAL_scene_lightSourcePatches = newListNode;

        GLOBAL_statistics_numberOfLightSources++;
    }
}

/**
Build the global light source patch list
*/
static void
mainBuildLightSourcePatchList() {
    GLOBAL_scene_lightSourcePatches = nullptr;
    GLOBAL_statistics_numberOfLightSources = 0;

    java::ArrayList<Patch *> *scenePatches = convertPatchSetToPatchList(globalAppScenePatches);
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        mainAddPatchToLightSourceListIfLightSource(scenePatches->get(i));
    }

    mainAddBackgroundToLightSourceList();
    GLOBAL_statistics_numberOfLightSources++;
}

static void
mainMakeRaytracingMethodsString() {
    char *str = globalRaytracingMethodsString;
    int n;
    snprintf(str, 80, "-raytracing-method <method>: set pixel-based radiance computation method\n%n", &n);
    str += n;
    snprintf(str, 80, "\tmethods: %-20.20s %s%s\n%n",
             "none", "no pixel-based radiance computation",
             !GLOBAL_raytracer_activeRaytracer ? " (default)" : "", &n);
    str += n;

    Raytracer **methodpp;
    for ( methodpp = globalRayTracingMethods; *methodpp; methodpp++ ) {
        Raytracer *method = *methodpp;
        snprintf(str, STRING_SIZE, "\t         %-20.20s %s%s\n%n",
                 method->shortName, method->fullName,
                 GLOBAL_raytracer_activeRaytracer == method ? " (default)" : "", &n);
        str += n;
    }
    *(str - 1) = '\0'; // Discard last newline character
}

static void
mainRayTracingDefaults() {
    for ( Raytracer **methodpp = globalRayTracingMethods; *methodpp; methodpp++ ) {
        Raytracer *method = *methodpp;
        method->Defaults();
        if ( strncasecmp(DEFAULT_RAYTRACING_METHOD, method->shortName, method->nameAbbrev) == 0 ) {
            mainSetRayTracingMethod(method);
        }
    }
    mainMakeRaytracingMethodsString(); // Comes last
}

static void
mainParseRayTracingOptions(int *argc, char **argv) {
    parseOptions(globalRaytracingOptions, argc, argv);
    for ( Raytracer **methodpp = globalRayTracingMethods; *methodpp; methodpp++ ) {
        Raytracer *method = *methodpp;
        method->ParseOptions(argc, argv);
    }
}

static void
mainRenderingDefaults() {
    RGB outlineColor = DEFAULT_OUTLINE_COLOR;
    RGB boundingBoxColor = DEFAULT_BOUNDING_BOX_COLOR;
    RGB clusterColor = DEFAULT_CLUSTER_COLOR;

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

    GLOBAL_render_renderOptions.draw_cameras = false;
    GLOBAL_render_renderOptions.camsize = 0.25;
    GLOBAL_render_renderOptions.linewidth = 1.0;
    GLOBAL_render_renderOptions.camera_color = GLOBAL_material_yellow;
    GLOBAL_render_renderOptions.render_raytraced_image = false;
    GLOBAL_render_renderOptions.use_background = true;
}

/**
Global initializations
*/
static void
mainInit() {
    // Transforms the cubature rules for quadrilaterals to be over the domain [0,1]^2 instead of [-1,1]^2
    fixCubatureRules();

    GLOBAL_fileOptions_monochrome = DEFAULT_MONOCHROME;
    GLOBAL_fileOptions_forceOneSidedSurfaces = DEFAULT_FORCE_ONESIDEDNESS;
    GLOBAL_fileOptions_numberOfQuarterCircleDivisions = DEFAULT_NQCDIVS;

    mainRenderingDefaults();
    toneMapDefaults();
    cameraDefaults();
    radianceDefaults(GLOBAL_scenePatches);
    mainRayTracingDefaults();

    // Default vertex compare flags: both location and normal is relevant. Two
    // vertices without normal, but at the same location, are to be considered
    // different
    vertexSetCompareFlags(VERTEX_COMPARE_LOCATION | VERTEX_COMPARE_NORMAL);

    // Specify what routines to be used to compare vertices when using
    // BREP_VERTEX_OCTREEs
    brepSetVertexCompareRoutine((BREP_COMPARE_FUNC) vertexCompare);
    brepSetVertexCompareLocationRoutine((BREP_COMPARE_FUNC) vertexCompareLocation);
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
mainCreateClusterHierarchy(PatchSet *patches) {
    Cluster *rootCluster;
    Geometry *rootGeometry;

    // Create a toplevel cluster containing (references to) all the patches in the solid
    rootCluster = new Cluster(patches);

    // Split the toplevel cluster recursively into sub-clusters
    rootCluster->splitCluster();

    // Convert to a Geometry GLOBAL_stochasticRaytracing_hierarchy, disposing of the clusters
    rootGeometry = rootCluster->convertClusterToGeom();

    delete rootCluster;
    return rootGeometry;
}

/**
Processes command line arguments not recognized by the Xt GUI toolkit
*/
static void
mainParseGlobalOptions(int *argc, char **argv) {
    GLOBAL_scenePatches = convertPatchSetToPatchList(globalAppScenePatches);

    parseRenderingOptions(argc, argv);
    parseToneMapOptions(argc, argv);
    parseCameraOptions(argc, argv);
    parseRadianceOptions(argc, argv);
    mainParseRayTracingOptions(argc, argv);
    parseBatchOptions(argc, argv);
    parseOptions(globalOptions, argc, argv); // Order is important, this should be called last
}

/**
Tries to read the scene in the given file. Returns false if not successful.
Returns true if successful. There's nothing GUI specific in this function.
When a file cannot be read, the current scene is restored
*/
static bool
mainReadFile(char *filename) {
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

    globalCurrentDirectory = (char *)malloc(n);
    snprintf(globalCurrentDirectory, n, "%s", filename);
    char *slash = strrchr(globalCurrentDirectory, '/');
    if ( slash != nullptr ) {
        *slash = '\0';
    } else {
        *globalCurrentDirectory = '\0';
    }

    logErrorReset();

    // Terminate any active radiance or raytracing methods
    fprintf(stderr, "Terminating current radiance/raytracing method ... \n");
    RADIANCEMETHOD *oRadiance = GLOBAL_radiance_currentRadianceMethodHandle;
    setRadianceMethod(nullptr, GLOBAL_scenePatches);
    Raytracer *oRayTracing = GLOBAL_raytracer_activeRaytracer;
    mainSetRayTracingMethod(nullptr);

    // Prepare if errors occur when reading the new scene will abort
    GLOBAL_scene_world = nullptr;
    if ( GLOBAL_scene_materials == nullptr ) {
        GLOBAL_scene_materials = new java::ArrayList<Material *>();
    }

    globalAppScenePatches = nullptr;
    patchSetNextId(1);
    GLOBAL_scene_clusteredWorld = nullptr;
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
        readMgf(filename);
    }

    clock_t t = clock();
    fprintf(stderr, "Reading took %g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    free(globalCurrentDirectory);

    // Check for errors
    if ( !GLOBAL_scene_world ) {
        return false; // Not successful
    }

    // Dispose of the old scene
    fprintf(stderr, "Disposing of the old scene ... ");
    fflush(stderr);
    if ( GLOBAL_radiance_currentRadianceMethodHandle ) {
        GLOBAL_radiance_currentRadianceMethodHandle->terminate(GLOBAL_scenePatches);
    }
    if ( GLOBAL_raytracer_activeRaytracer ) {
        GLOBAL_raytracer_activeRaytracer->Terminate();
    }

    PatchSet *listWindow = globalAppScenePatches;
    while ( listWindow != nullptr ) {
        PatchSet *next = listWindow->next;
        free(listWindow);
        listWindow = next;
    }

    listWindow = GLOBAL_scene_lightSourcePatches;
    while ( listWindow != nullptr ) {
        PatchSet *next = listWindow->next;
        free(listWindow);
        listWindow = next;
    }

    if ( GLOBAL_scene_clusteredWorldGeom ) {
        geomDestroy(GLOBAL_scene_clusteredWorldGeom);
        GLOBAL_scene_clusteredWorldGeom = nullptr;
    }
    if ( GLOBAL_scene_background != nullptr ) {
        GLOBAL_scene_background->methods->Destroy(GLOBAL_scene_background->data);
        GLOBAL_scene_background = nullptr;
    }

    if ( GLOBAL_scene_worldVoxelGrid != nullptr ) {
        GLOBAL_scene_worldVoxelGrid->destroyGrid();
        GLOBAL_scene_worldVoxelGrid = nullptr;
    }

    if ( GLOBAL_scene_materials != nullptr ) {
        // Note: it seems this should not be called - are materials linked to other structures?
        //for ( int i; i < GLOBAL_scene_materials->size(); i++ ) {
        //    MaterialDestroy(GLOBAL_scene_materials->get(i));
        //}
        delete GLOBAL_scene_materials;
        GLOBAL_scene_materials = nullptr;
    }

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Build the new patch list, this is duplicating already available
    // information and as such potentially dangerous, but we need it
    // so many times
    fprintf(stderr, "Building patch list ... ");
    fflush(stderr);

    globalAppScenePatches = buildPatchList(GLOBAL_scene_world, nullptr /*should replace with new list*/);

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
    if ( GLOBAL_scene_clusteredWorldGeom->methods == &GLOBAL_skin_compoundGeometryMethods ) {
        if ( GLOBAL_scene_clusteredWorldGeom->compoundData != nullptr ) {
            fprintf(stderr, "Unexpected case: review code - aggregate is not compound.\n");
            exit(1);
        } else if ( GLOBAL_scene_clusteredWorldGeom->aggregateData != nullptr ) {
            GLOBAL_scene_clusteredWorld = GLOBAL_scene_clusteredWorldGeom->aggregateData;
        } else {
            fprintf(stderr, "Unexpected case: review code - generic case not supported anymore.\n");
            exit(2);
        }
    } else {
        // Small memory leak here ... but exceptional situation!
        GLOBAL_scene_clusteredWorld = geometryListAdd(nullptr, GLOBAL_scene_clusteredWorldGeom);
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

    GLOBAL_statistics_numberOfPatches = GLOBAL_statistics_numberOfElements;
    mainComputeSomeSceneStats();
    GLOBAL_statistics_referenceLuminance = 5.42 * ((1. - colorGray(GLOBAL_statistics_averageReflectivity)) *
            colorLuminance(GLOBAL_statistics_estimatedAverageRadiance));

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Initialize tone mapping
    fprintf(stderr, "Initializing tone mapping ... ");
    fflush(stderr);

    initToneMapping(convertPatchSetToPatchList(globalAppScenePatches));

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    printf("Stats: GLOBAL_statistics_totalEmittedPower .............: %f W\n"
           "       estimated_average_illuminance ...: %f W/sr\n"
           "       GLOBAL_statistics_averageReflectivity ............: %f\n"
           "       GLOBAL_statistics_maxSelfEmittedRadiance ........: %f W/sr\n"
           "       GLOBAL_statistics_maxSelfEmittedPower ...........: %f W\n"
           "       adaptation_luminance ............: %f cd/m2\n"
           "       GLOBAL_statistics_totalArea ......................: %f m2\n",
           colorGray(GLOBAL_statistics_totalEmittedPower),
           colorGray(GLOBAL_statistics_estimatedAverageRadiance),
           colorGray(GLOBAL_statistics_averageReflectivity),
           colorGray(GLOBAL_statistics_maxSelfEmittedRadiance),
           colorGray(GLOBAL_statistics_maxSelfEmittedPower),
           GLOBAL_toneMap_options.lwa,
           GLOBAL_statistics_totalArea);

    // Initialize radiance for the freshly loaded scene
    if ( oRadiance ) {
        fprintf(stderr, "Initializing radiance computations ... ");
        fflush(stderr);

        setRadianceMethod(oRadiance, convertPatchSetToPatchList(globalAppScenePatches));

        t = clock();
        fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
        last = t;
    }

    if ( oRayTracing ) {
        fprintf(stderr, "Initializing raytracing computations ... \n");

        mainSetRayTracingMethod(oRayTracing);

        t = clock();
        fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    }

    // Remove possible render hooks
    removeAllRenderHooks();

    fprintf(stderr, "Initialisations done.\n");

    return true;
}

static void
mainBuildModel(const int *argc, char *const *argv) {
    // All options should have disappeared from argv now
    if ( *argc > 1 ) {
        if ( *argv[1] == '-' ) {
            logError(nullptr, "Unrecognized option '%s'", argv[1]);
        } else if ( !mainReadFile(argv[1]) ) {
                exit(1);
            }
    }
}

int
main(int argc, char *argv[]) {
    mainInit();
    mainParseGlobalOptions(&argc, argv);
    mainBuildModel(&argc, argv);
    mainExecuteRendering(GLOBAL_scenePatches);
    return 0;
}
