#include <ctime>
#include <cstring>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "io/mgf/readmgf.h"
#include "app/sceneBuilder.h"
#include "material/statistics.h"
#include "app/Cluster.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "render/renderhook.h"

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
mainComputeSomeSceneStats(Scene *scene) {
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
    for ( int i = 0; i < scene->patchList->size(); i++ ) {
        mainPatchAccumulateStats(scene->patchList->get(i));
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
    BP = backgroundPower(scene->background, &zero);
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
Adds the background to the global light source patch list
*/
static void
mainAddBackgroundToLightSourceList(Scene *scene) {
    if ( scene != nullptr && scene->background != nullptr && scene->background->bkgPatch != nullptr ) {
        scene->lightSourcePatchList->add(scene->background->bkgPatch);
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
static void
mainBuildLightSourcePatchList(Scene *scene) {
    scene->lightSourcePatchList = new java::ArrayList<Patch *>();
    GLOBAL_statistics.numberOfLightSources = 0;

    for ( int i = 0; i < scene->patchList->size(); i++ ) {
        mainAddPatchToLightSourceListIfLightSource(scene->lightSourcePatchList, scene->patchList->get(i));
    }

    mainAddBackgroundToLightSourceList(scene);
}

/**
Tries to read the scene in the given file. Returns false if not successful.
Returns true if successful. There's nothing GUI specific in this function.
When a file cannot be read, the current scene is restored
*/
static bool
mainReadFile(char *filename, MgfContext *context, Scene *scene, char **currentWorkingDirectory) {
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

    *currentWorkingDirectory = new char[n];
    snprintf(*currentWorkingDirectory, n, "%s", filename);
    char *slash = strrchr(*currentWorkingDirectory, '/');
    if ( slash != nullptr ) {
        *slash = '\0';
    } else {
        *currentWorkingDirectory[0] = '\0';
    }

    // Init compute method
    setRadianceMethod(nullptr, scene->camera, nullptr, scene->clusteredRootGeometry);

#ifdef RAYTRACING_ENABLED
    Raytracer *currentRaytracer = GLOBAL_raytracer_activeRaytracer;
        mainSetRayTracingMethod(nullptr, nullptr);
#endif

    // Prepare if errors occur when reading the new scene will abort
    scene->geometryList = nullptr;

    Patch::setNextId(1);
    scene->clusteredGeometryList = new java::ArrayList<Geometry *>();
    scene->background = nullptr;

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
        scene->geometryList = context->geometries;
    }

    clock_t t = clock();
    fprintf(stderr, "Reading took %g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Check for errors
    if ( scene->geometryList == nullptr || scene->geometryList->size() == 0 ) {
        return false; // Not successful
    }

    // Build the new patch list, this is duplicating already available
    // information and as such potentially dangerous, but we need it
    // so many times
    fprintf(stderr, "Building patch list ... ");
    fflush(stderr);

    scene->patchList = new java::ArrayList<Patch *>();
    buildPatchList(scene->geometryList, scene->patchList);

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Build the list of patches on light sources from the patch list
    fprintf(stderr, "Building light source patch list ... ");
    fflush(stderr);

    mainBuildLightSourcePatchList(scene);

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Build a cluster hierarchy for the new scene
    fprintf(stderr, "Building cluster hierarchy ... ");
    fflush(stderr);

    scene->clusteredRootGeometry = mainCreateClusterHierarchy(scene->patchList);

    if ( scene->clusteredRootGeometry->className == GeometryClassId::COMPOUND ) {
        if ( scene->clusteredRootGeometry->compoundData == nullptr ) {
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
    scene->voxelGrid = new VoxelGrid(scene->clusteredRootGeometry);

    t = clock();
    fprintf(stderr, "Voxel grid creation took %g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Estimate average radiance, for radiance to display RGB conversion
    fprintf(stderr, "Computing some scene statistics ... ");
    fflush(stderr);

    GLOBAL_statistics.numberOfPatches = GLOBAL_statistics.numberOfElements;
    mainComputeSomeSceneStats(scene);
    GLOBAL_statistics.referenceLuminance = 5.42 * ((1.0 - GLOBAL_statistics.averageReflectivity.gray()) *
                                                   GLOBAL_statistics.estimatedAverageRadiance.luminance());

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
    last = t;

    // Initialize tone mapping
    fprintf(stderr, "Initializing tone mapping ... ");
    fflush(stderr);

    initToneMapping(scene->patchList);

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

    setRadianceMethod(context->radianceMethod, scene->camera, scene->patchList, scene->clusteredRootGeometry);

    t = clock();
    fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);

#ifdef RAYTRACING_ENABLED
    if ( currentRaytracer != nullptr ) {
            fprintf(stderr, "Initializing raytracing method ... \n");

            mainSetRayTracingMethod(currentRaytracer, scene->lightSourcePatchList);

            t = clock();
            fprintf(stderr, "%g secs.\n", (float) (t - last) / (float) CLOCKS_PER_SEC);
        }
#endif

    // Remove possible render hooks
    removeAllRenderHooks();

    fprintf(stderr, "Initialisations done.\n");

    return true;
}

void
mainBuildModel(
    const int *argc,
    char *const *argv,
    MgfContext *context,
    Scene *scene,
    char **currentWorkingDirectory)
{
    // All options should have disappeared from argv now
    if ( *argc > 1 ) {
        if ( *argv[1] == '-' ) {
            logError(nullptr, "Unrecognized option '%s'", argv[1]);
        } else if ( !mainReadFile(argv[1], context, scene, currentWorkingDirectory) ) {
            exit(1);
        }
    }
}
