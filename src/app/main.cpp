#include <cstdlib>
#include <ctime>
#include <cstring>

#include "common/error.h"
#include "BREP/BREP_VERTEX_OCTREE.h"
#include "material/statistics.h"
#include "shared/defaults.h"
#include "shared/options.h"
#include "shared/render.h"
#include "shared/cubature.h"
#include "shared/renderhook.h"
#include "scene/scene.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "raycasting/simple/RayCaster.h"
#include "raycasting/simple/RayMatter.h"
#include "raycasting/raytracing/BidirectionalPathRaytracer.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracer.h"
#include "app/Cluster.h"
#include "app/ui.h"
#include "io/mgf/readmgf.h"
#include "skin/Compound.h"
#include "app/fileopts.h"

static char *currentDirectory;
static int yes = 1;
static int no = 0;

#define GeomIsCompound(geom) (geom->methods == &GLOBAL_skin_compoundGeometryMethods)

static void
PatchAccumulateStats(PATCH *patch) {
    COLOR
            E = PatchAverageEmittance(patch, ALL_COMPONENTS),
            R = PatchAverageNormalAlbedo(patch, BSDF_ALL_COMPONENTS),
            power;

    GLOBAL_statistics_totalArea += patch->area;
    colorScale(patch->area, E, power);
    colorAdd(GLOBAL_statistics_totalEmittedPower, power, GLOBAL_statistics_totalEmittedPower);
    colorAddScaled(GLOBAL_statistics_averageReflectivity, patch->area, R, GLOBAL_statistics_averageReflectivity);
    /* convert radiant exitance to exitant radiance */
    colorScale((1.0 / (float)M_PI), E, E);
    colorMaximum(E, GLOBAL_statistics_maxSelfEmittedRadiance, GLOBAL_statistics_maxSelfEmittedRadiance);
    colorMaximum(power, GLOBAL_statistics_maxSelfEmittedPower, GLOBAL_statistics_maxSelfEmittedPower);
}

static void
ComputeSomeSceneStats() {
    Vector3D zero;
    COLOR one, average_absorption, BP;

    colorSetMonochrome(one, 1.0f);
    VECTORSET(zero, 0, 0, 0);

    /* initialize */
    colorClear(GLOBAL_statistics_totalEmittedPower);
    colorClear(GLOBAL_statistics_averageReflectivity);
    colorClear(GLOBAL_statistics_maxSelfEmittedRadiance);
    colorClear(GLOBAL_statistics_maxSelfEmittedPower);
    GLOBAL_statistics_totalArea = 0.;

    /* accumulate */
    PatchListIterate(GLOBAL_scene_patches, PatchAccumulateStats);

    /* averages ... */
    colorScaleInverse(GLOBAL_statistics_totalArea, GLOBAL_statistics_averageReflectivity, GLOBAL_statistics_averageReflectivity);
    colorSubtract(one, GLOBAL_statistics_averageReflectivity, average_absorption);
    colorScaleInverse(M_PI * GLOBAL_statistics_totalArea, GLOBAL_statistics_totalEmittedPower, GLOBAL_statistics_estimatedAverageRadiance);

    /* include background radiation */
    BP = BackgroundPower(GLOBAL_scene_background, &zero);
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
AddBackgroundToLightSourceList() {
    if ( GLOBAL_scene_background ) {
        GLOBAL_scene_lightSourcePatches = PatchListAdd(GLOBAL_scene_lightSourcePatches, GLOBAL_scene_background->bkgPatch);
    }
}


/**
Adds the patch to the global light source patch list if the patch is on 
a light source (i.e. when the surfaces material has a non-null edf)
*/
static void
AddPatchToLightSourceListIfLightSource(PATCH *patch) {
    if ( patch->surface->material->edf ) {
        GLOBAL_scene_lightSourcePatches = PatchListAdd(GLOBAL_scene_lightSourcePatches, patch);
        GLOBAL_statistics_numberOfLightSources++;
    }
}

/**
Build the global light source patch list
*/
static void
BuildLightSourcePatchList() {
    GLOBAL_scene_lightSourcePatches = PatchListCreate();
    GLOBAL_statistics_numberOfLightSources = 0;
    PatchListIterate(GLOBAL_scene_patches, AddPatchToLightSourceListIfLightSource);

    AddBackgroundToLightSourceList();
    GLOBAL_statistics_numberOfLightSources++;
}

/**
Table of available raytracing methods
*/
static Raytracer *RayTracingMethods[] = {
    &RT_StochasticMethod,
    &RT_BidirPathMethod,
    &RayCasting,
    &RayMatting,
    (Raytracer *)nullptr
};

/**
Iterator over all available raytracing methods
*/
#define ForAllRayTracingMethods(methodp) {{ \
  Raytracer **methodpp; \
  for (methodpp=RayTracingMethods; *methodpp; methodpp++) { \
    Raytracer *methodp = *methodpp;

static char raytracing_methods_string[1000];

static void
make_raytracing_methods_string() {
    char *str = raytracing_methods_string;
    int n;
    sprintf(str, "\
-raytracing-method <method>: Set pixel-based radiance computation method\n%n",
            &n);
    str += n;
    sprintf(str, "\tmethods: %-20.20s %s%s\n%n",
            "none", "no pixel-based radiance computation",
            !Global_Raytracer_activeRaytracer ? " (default)" : "", &n);
    str += n;
    ForAllRayTracingMethods(method)
                {
                    sprintf(str, "\t         %-20.20s %s%s\n%n",
                            method->shortName, method->fullName,
                            Global_Raytracer_activeRaytracer == method ? " (default)" : "", &n);
                    str += n;
                }
    EndForAll;
    *(str - 1) = '\0';    /* discard last newline character */
}

static void
RayTracingDefaults() {
    ForAllRayTracingMethods(method)
                {
                    method->Defaults();
                    if ( strncasecmp(DEFAULT_RAYTRACING_METHOD, method->shortName, method->nameAbbrev) == 0 ) {
                        SetRayTracing(method);
                    }
                }
    EndForAll;
    make_raytracing_methods_string();    /* comes last */
}

static void
RayTracingOption(void *value) {
    char *name = *(char **) value;

    ForAllRayTracingMethods(method)
                {
                    if ( strncasecmp(name, method->shortName, method->nameAbbrev) == 0 ) {
                        SetRayTracing(method);
                        return;
                    }
                }
    EndForAll;

    if ( strncasecmp(name, "none", 4) == 0 ) {
        SetRayTracing((Raytracer *) nullptr);
    } else {
        Error(nullptr, "Invalid raytracing method name '%s'", name);
    }
}

/**
String describing the -raytracing-method command line option
*/
static CMDLINEOPTDESC raytracingOptions[] = {
    {"-raytracing-method", 4, Tstring,  nullptr, RayTracingOption,
     raytracing_methods_string},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

static void
ParseRayTracingOptions(int *argc, char **argv) {
    ParseOptions(raytracingOptions, argc, argv);
    ForAllRayTracingMethods(method)
                {
                    method->ParseOptions(argc, argv);
                }
    EndForAll;
}

static void
RenderingDefaults() {
    RGB outlinecolor = DEFAULT_OUTLINE_COLOR;
    RGB bbcolor = DEFAULT_BOUNDING_BOX_COLOR;
    RGB cluscolor = DEFAULT_CLUSTER_COLOR;

    RenderUseDisplayLists(DEFAULT_DISPLAY_LISTS);
    RenderSetSmoothShading(DEFAULT_SMOOTH_SHADING);
    RenderSetBackfaceCulling(DEFAULT_BACKFACE_CULLING);
    RenderSetOutlineDrawing(DEFAULT_OUTLINE_DRAWING);
    RenderSetBoundingBoxDrawing(DEFAULT_BOUNDING_BOX_DRAWING);
    RenderSetClusterDrawing(DEFAULT_CLUSTER_DRAWING);
    RenderSetOutlineColor(&outlinecolor);
    RenderSetBoundingBoxColor(&bbcolor);
    RenderSetClusterColor(&cluscolor);
    RenderUseFrustumCulling(true);

    RenderSetNoShading(false);

    renderopts.draw_cameras = false;
    renderopts.camsize = 0.25;
    renderopts.linewidth = 1.0;
    renderopts.camera_color = GLOBAL_material_yellow;

    renderopts.render_raytraced_image = false;
    renderopts.use_background = true;
}


/**
Global initializations
*/
static void
Init() {
    /* Transforms the cubature rules for quadrilaterals to be over the domain [0,1]^2
     * instead of [-1,1]^2. See cubature.[ch] */
    FixCubatureRules();

    monochrome = DEFAULT_MONOCHROME;
    force_onesided_surfaces = DEFAULT_FORCE_ONESIDEDNESS;
    nqcdivs = DEFAULT_NQCDIVS;

    RenderingDefaults();
    ToneMapDefaults();
    CameraDefaults();
    RadianceDefaults();
    RayTracingDefaults();

    /* Default vertex compare flags: both location and normal is relevant. Two
     * vertices without normal, but at the same location, are to be considered
     * different. */
    VertexSetCompareFlags(VCMP_LOCATION | VCMP_NORMAL);

    /* Specify what routines to be used to compare vertices when using
     * BREP_VERTEX_OCTREEs */
    brepSetVertexCompareRoutine((BREP_COMPARE_FUNC) VertexCompare);
    brepSetVertexCompareLocationRoutine((BREP_COMPARE_FUNC) VertexCompareLocation);
}

static void
ForceOnesidedOption(void *value) {
    force_onesided_surfaces = *(int *) value;
}

static void
MonochromeOption(void *value) {
    monochrome = *(int *) value;
}

static CMDLINEOPTDESC globalOptions[] = {
    {"-nqcdivs", 3, Tint, &nqcdivs, DEFAULT_ACTION,
     "-nqcdivs <integer>\t: number of quarter circle divisions"},
    {"-force-onesided", 10, TYPELESS, &yes, ForceOnesidedOption,
     "-force-onesided\t\t: force one-sided surfaces"},
    {"-dont-force-onesided", 14, TYPELESS, &no, ForceOnesidedOption,
     "-dont-force-onesided\t: allow two-sided surfaces"},
    {"-monochromatic", 5, TYPELESS, &yes, MonochromeOption,
     "-monochromatic \t\t: convert colors to shades of grey"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

/**
Creates a hierarchical model of the discrete scene (the patches in the scene) using the simple
algorithm described in
- Per Christensen, "Hierarchical Techniques for Glossy Global Illumination",
  PhD Thesis, University of Washington, 1995, p 116
This hierarchy is often much more efficient for tracing rays and clustering radiosity algorithms
than the given hierarchy of bounding boxes. A pointer to the toplevel "cluster" is returned
*/
static Geometry *
createClusterHierarchy(PatchSet *patches) {
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
ParseGlobalOptions(int *argc, char **argv) {
    ParseRenderingOptions(argc, argv);
    ParseToneMapOptions(argc, argv);
    ParseCameraOptions(argc, argv);
    ParseRadianceOptions(argc, argv);
    ParseRayTracingOptions(argc, argv);
    ParseInterfaceOptions(argc, argv);
    ParseOptions(globalOptions, argc, argv);    /* this one comes last in order to
						 * have all other options parsed before
						 * reading input from stdin. */
}

/**
Tries to read the scene in the given file. Returns false if not succesful.
Returns true if succesful. There's nothing GUI specific in this function. 
When a file cannot be read, the current scene is restored
*/
bool
ReadFile(char *filename) {
    char *dot, *slash, *extension;
    FILE *input;
    GeometryListNode *oWorld, *oClusteredWorld;
    Geometry *oClusteredWorldGeom;
    MATERIALLIST *oMaterialLib;
    PatchSet *oPatches, *oLightSourcePatches;
    VoxelGrid *oWorldGrid;
    RADIANCEMETHOD *oRadiance;
    Raytracer *oRayTracing;
    Background *oBackground;
    int opatchid, onrpatches;
    clock_t t, last;

    // check whether the file can be opened if not reading from stdin
    if ( filename[0] != '#' ) {
        if ((input = fopen(filename, "r")) == (FILE *) nullptr ||
            fgetc(input) == EOF) {
            if ( input ) {
                fclose(input);
            }
            Error(nullptr, "Can't open file '%s' for reading", filename);
            return false;
        }
        fclose(input);
    }

    // get current directory from the filename
    currentDirectory = (char *)malloc(strlen(filename) + 1);
    sprintf(currentDirectory, "%s", filename);
    if ((slash = strrchr(currentDirectory, '/')) != nullptr ) {
        *slash = '\0';
    } else {
        *currentDirectory = '\0';
    }

    ErrorReset();

    // terminate any active radiance or raytracing methods
    fprintf(stderr, "Terminating current radiance/raytracing method ... \n");
    oRadiance = Radiance;
    SetRadianceMethod((RADIANCEMETHOD *) nullptr);
    oRayTracing = Global_Raytracer_activeRaytracer;
    SetRayTracing((Raytracer *) nullptr);

    // save the current scene so it can be restored if errors occur when reading the new scene
    fprintf(stderr, "Saving current scene ... \n");
    oWorld = GLOBAL_scene_world;
    GLOBAL_scene_world = GeomListCreate();
    oMaterialLib = GLOBAL_scene_materials;
    GLOBAL_scene_materials = MaterialListCreate();
    oPatches = GLOBAL_scene_patches;
    GLOBAL_scene_patches = PatchListCreate();
    opatchid = PatchGetNextID();
    PatchSetNextID(1);
    onrpatches = GLOBAL_statistics_numberOfPatches;
    oClusteredWorld = GLOBAL_scene_clusteredWorld;
    GLOBAL_scene_clusteredWorld = GeomListCreate();
    oClusteredWorldGeom = GLOBAL_scene_clusteredWorldGeom;
    oLightSourcePatches = GLOBAL_scene_lightSourcePatches;
    oWorldGrid = GLOBAL_scene_worldGrid;
    oBackground = GLOBAL_scene_background;
    GLOBAL_scene_background = (Background *) nullptr;

    // Read the mgf file. The result is a new GLOBAL_scene_world and GLOBAL_scene_materials if everything goes well
    fprintf(stderr, "Reading the scene from file '%s' ... \n", filename);
    last = clock();

    if ((dot = strrchr(filename, '.')) != nullptr ) {
        extension = dot + 1;
    } else {
        extension = (char *) "mgf";
    }

    if ( strncmp(extension, "mgf", 3) == 0 ) {
        ReadMgf(filename);
    }

    t = clock();
    fprintf(stderr, "Reading took %g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    free(currentDirectory);
    currentDirectory = nullptr;

    // Check for errors
    if ( !GLOBAL_scene_world ) {
        // Restore the old scene
        fprintf(stderr, "Restoring old scene ... ");
        fflush(stderr);
        GeomListIterate(GLOBAL_scene_world, geomDestroy);
        GeomListDestroy(GLOBAL_scene_world);
        GLOBAL_scene_world = oWorld;

        MaterialListIterate(GLOBAL_scene_materials, MaterialDestroy);
        MaterialListDestroy(GLOBAL_scene_materials);
        GLOBAL_scene_materials = oMaterialLib;

        GLOBAL_scene_patches = oPatches;
        PatchSetNextID(opatchid);
        GLOBAL_statistics_numberOfPatches = onrpatches;

        GLOBAL_scene_clusteredWorld = oClusteredWorld;
        GLOBAL_scene_clusteredWorldGeom = oClusteredWorldGeom;
        GLOBAL_scene_lightSourcePatches = oLightSourcePatches;

        GLOBAL_scene_worldGrid = oWorldGrid;
        GLOBAL_scene_background = oBackground;

        SetRadianceMethod(oRadiance);
        SetRayTracing(oRayTracing);

        t = clock();
        fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
        last = t;
        fprintf(stderr, "Done.\n");

        if ( !ErrorOccurred()) {
            Error(nullptr, "Empty world");
        }

        return false; // not succesful
    }

    // Dispose of the old scene
    fprintf(stderr, "Disposing of the old scene ... ");
    fflush(stderr);
    if ( Radiance ) {
        Radiance->Terminate();
    }
    if ( Global_Raytracer_activeRaytracer ) {
        Global_Raytracer_activeRaytracer->Terminate();
    }

    PatchListDestroy(oPatches);
    PatchListDestroy(oLightSourcePatches);

    GeomListIterate(oWorld, geomDestroy);
    GeomListDestroy(oWorld);

    if ( oClusteredWorldGeom ) {
        geomDestroy(oClusteredWorldGeom);
    }
    if ( oBackground ) {
        oBackground->methods->Destroy(oBackground->data);
    }

    destroyGrid(oWorldGrid);

    MaterialListIterate(oMaterialLib, MaterialDestroy);
    MaterialListDestroy(oMaterialLib);

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    /* build the new patch list, this is duplicating already available
     * information and as such potentially dangerous, but we need it
     * so many times, so ... */
    fprintf(stderr, "Building patch list ... ");
    fflush(stderr);

    GLOBAL_scene_patches = BuildPatchList(GLOBAL_scene_world, PatchListCreate());

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Destilate the list of patches on light sources from the above patch list
    fprintf(stderr, "Building light source patch list ... ");
    fflush(stderr);

    BuildLightSourcePatchList();

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Build a cluster hierarchy for the new scene
    fprintf(stderr, "Building cluster hierarchy ... ");
    fflush(stderr);

    GLOBAL_scene_clusteredWorldGeom = createClusterHierarchy(GLOBAL_scene_patches);
    if ( GeomIsCompound(GLOBAL_scene_clusteredWorldGeom)) {
        GLOBAL_scene_clusteredWorld = (GeometryListNode *) (GLOBAL_scene_clusteredWorldGeom->obj);
    } else {
        // small memory leak here ... but exceptional situation!
        GLOBAL_scene_clusteredWorld = GeomListAdd(GeomListCreate(), GLOBAL_scene_clusteredWorldGeom);
        Warning(nullptr, "Strange clusters for this world ...");
    }

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // engridding the thing
    GLOBAL_scene_worldGrid = createGrid(GLOBAL_scene_clusteredWorldGeom);

    t = clock();
    fprintf(stderr, "Engridding took %g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Estimate average radiance, for radiance to display RGB conversion
    fprintf(stderr, "Computing some scene statistics ... ");
    fflush(stderr);

    GLOBAL_statistics_numberOfPatches = GLOBAL_statistics_numberOfElements;
    ComputeSomeSceneStats();
    GLOBAL_statistics_referenceLuminance = 5.42 * ((1. - colorGray(GLOBAL_statistics_averageReflectivity)) *
            colorLuminance(GLOBAL_statistics_estimatedAverageRadiance));

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Initialize tone mapping
    fprintf(stderr, "Initializing tone mapping ... ");
    fflush(stderr);

    InitToneMapping();

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
           tmopts.lwa,
           GLOBAL_statistics_totalArea);

    // initialize radiance for the freshly loaded scene
    if ( oRadiance ) {
        fprintf(stderr, "Initializing radiance computations ... ");
        fflush(stderr);

        SetRadianceMethod(oRadiance);

        t = clock();
        fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
        last = t;
    }

    if ( oRayTracing ) {
        fprintf(stderr, "Initializing raytracing computations ... \n");

        SetRayTracing(oRayTracing);

        t = clock();
        fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
        last = t;
    }

    // Remove possible renderhooks
    RemoveAllRenderHooks();

    fprintf(stderr, "Initialisations done.\n");

    return true;
}

int
main(int argc, char *argv[]) {
    Init();
    ParseGlobalOptions(&argc, argv);
    StartUserInterface(&argc, argv);
    return 0;
}
