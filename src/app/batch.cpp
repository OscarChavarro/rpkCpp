#include <ctime>
#include <cstring>

#include "common/error.h"
#include "material/statistics.h"
#include "skin/radianceinterfaces.h"
#include "scene/scene.h"
#include "shared/writevrml.h"
#include "shared/options.h"
#include "shared/canvas.h"
#include "shared/render.h"
#include "io/FileUncompressWrapper.h"
#include "raycasting/common/Raytracer.h"
#include "app/opengl.h"
#include "app/batch.h"

static int globalIterations = 1; // Radiance method iterations
static int globalSaveModulo = 10; // Every 10th iteration, surface model and image will be saved
static int globalTimings = false;
static const char *globalRadianceImageFileNameFormat = "";
static const char *globalRadianceModelFileNameFormat = "";
static const char *globalRaytracingImageFileName = "";

static CommandLineOptionDescription batchOptions[] = {
    {"-iterations", 3,  Tint, &globalIterations, DEFAULT_ACTION,
     "-iterations <integer>\t: world-space radiance iterations"},
    {"-radiance-image-savefile", 12, Tstring,
     &globalRadianceImageFileNameFormat, DEFAULT_ACTION,
     "-radiance-image-savefile <filename>\t: radiance PPM/LOGLUV savefile name,"
     "\n\tfirst '%%d' will be substituted by iteration number"},
    {"-radiance-model-savefile", 12, Tstring,
     &globalRadianceModelFileNameFormat, DEFAULT_ACTION,
     "-radiance-model-savefile <filename>\t: radiance VRML model savefile name,"
     "\n\tfirst '%%d' will be substituted by iteration number"},
    {"-save-modulo", 8, Tint, &globalSaveModulo, DEFAULT_ACTION,
     "-save-modulo <integer>\t: save every n-th iteration"},
    {"-raytracing-image-savefile", 14, Tstring, &globalRaytracingImageFileName, DEFAULT_ACTION,
     "-raytracing-image-savefile <filename>\t: raytracing PPM savefile name"},
    {"-timings", 3,  Tsettrue, &globalTimings, DEFAULT_ACTION,
     "-timings\t: printRegularHierarchy timings for world-space radiance and raytracing methods"},
    {nullptr, 0,  TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

/**
This routine was copied from uit.c, leaving out all interface related things
*/
static void
batchProcessFile(
    const char *fileName,
    const char *open_mode,
    void (*processFileCallback)(const char *fileName, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches),
    java::ArrayList<Patch *> *scenePatches)
{
    int isPipe;
    FILE *fp = openFile(fileName, open_mode, &isPipe);

    // Call the user supplied procedure to process the file
    processFileCallback(fileName, fp, isPipe, scenePatches);

    closeFile(fp, isPipe);
}

static void
batchSaveRadianceImage(const char *fileName, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches) {
    clock_t t;
    char *extension;

    if ( !fp ) {
        return;
    }

    canvasPushMode();

    extension = imageFileExtension((char *) fileName);
    if ( IS_TIFF_LOGLUV_EXT(extension)) {
        fprintf(stdout, "Saving LOGLUV image to file '%s' ....... ", fileName);
    } else {
        fprintf(stdout, "Saving RGB image to file '%s' .......... ", fileName);
    }
    fflush(stdout);

    t = clock();

    saveScreen((char *) fileName, fp, isPipe, scenePatches);

    fprintf(stdout, "%g secs.\n", (float) (clock() - t) / (float) CLOCKS_PER_SEC);
    canvasPullMode();
}

static void
batchSaveRadianceModel(const char *fileName, FILE *fp, int /*isPipe*/, java::ArrayList<Patch *> *scenePatches) {
    clock_t t;

    if ( !fp ) {
        return;
    }

    canvasPushMode();
    fprintf(stdout, "Saving VRML model to file '%s' ... ", fileName);
    fflush(stdout);
    t = clock();

    if ( GLOBAL_radiance_currentRadianceMethodHandle && GLOBAL_radiance_currentRadianceMethodHandle->writeVRML != nullptr ) {
        GLOBAL_radiance_currentRadianceMethodHandle->writeVRML(fp);
    } else {
        writeVRML(fp, scenePatches);
    }

    fprintf(stdout, "%g secs.\n", (float) (clock() - t) / (float) CLOCKS_PER_SEC);
    canvasPullMode();
}

static void
batchSaveRaytracingImage(const char *fileName, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches) {
    ImageOutputHandle *img = nullptr;
    clock_t t;

    if ( !fp ) {
        return;
    }

    t = clock();

    if ( fp ) {
        img = createRadianceImageOutputHandle(
                (char *) fileName,
                fp,
                isPipe,
                GLOBAL_camera_mainCamera.xSize,
                GLOBAL_camera_mainCamera.ySize,
                GLOBAL_statistics_referenceLuminance / 179.0f);
        if ( !img ) {
            return;
        }
    }

    if ( !GLOBAL_raytracer_activeRaytracer ) {
        logWarning(nullptr, "No ray tracing method active");
    } else if ( !GLOBAL_raytracer_activeRaytracer->SaveImage || !GLOBAL_raytracer_activeRaytracer->SaveImage(img)) {
        logWarning(nullptr, "No previous %s image available", GLOBAL_raytracer_activeRaytracer->fullName);
    }

    if ( img != nullptr ) {
        deleteImageOutputHandle(img);
    }

    fprintf(stdout, "Raytrace save image: %g secs.\n", (float) (clock() - t) / (float) CLOCKS_PER_SEC);

    return;
}

static void
batchRayTrace(char *filename, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches) {
    GLOBAL_render_renderOptions.render_raytraced_image = true;
    GLOBAL_camera_mainCamera.changed = false;

    canvasPushMode();
    rayTrace(filename, fp, isPipe, GLOBAL_raytracer_activeRaytracer, scenePatches, lightPatches);
    canvasPullMode();
}

void
parseBatchOptions(int *argc, char **argv) {
    parseOptions(batchOptions, argc, argv);
}

void
batch(java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches) {
    clock_t start_time, wasted_start;
    float wasted_secs;

    if ( !GLOBAL_scene_world ) {
        printf("Empty world??\n");
        return;
    }

    start_time = clock();
    wasted_secs = 0.0;

    if ( GLOBAL_radiance_currentRadianceMethodHandle ) {
        // GLOBAL_scene_world-space radiance computations
        int it = 0;
        bool done = false;

        printf("Doing %s ...\n", GLOBAL_radiance_currentRadianceMethodHandle->fullName);

        fflush(stdout);
        fflush(stderr);

        while ( !done ) {
            printf("-----------------------------------\n"
                   "GLOBAL_scene_world-space radiance iteration %04d\n"
                   "-----------------------------------\n\n", it);

            canvasPushMode();
            done = GLOBAL_radiance_currentRadianceMethodHandle->doStep(scenePatches, lightPatches);
            canvasPullMode();

            fflush(stdout);
            fflush(stderr);

            printf("%s", GLOBAL_radiance_currentRadianceMethodHandle->GetStats());

            fflush(stdout);
            fflush(stderr);

            openGlRenderScene(scenePatches);

            fflush(stdout);
            fflush(stderr);

            wasted_start = clock();

            if ( (!(it % globalSaveModulo)) && *globalRadianceImageFileNameFormat ) {
                int n = strlen(globalRadianceImageFileNameFormat) + 1;
                char *fileName = new char[n];
                snprintf(fileName, n, globalRadianceImageFileNameFormat, it);
                if ( GLOBAL_render_renderOptions.trace ) {
                    batchProcessFile(fileName, "w", batchSaveRadianceImage, scenePatches);
                } else {
                    batchProcessFile(fileName, "w", batchSaveRadianceImage, scenePatches);
                }
                delete[] fileName;
            }

            if ( *globalRadianceModelFileNameFormat ) {
                int n = strlen(globalRadianceModelFileNameFormat) + 1;
                char *fileName = new char[n];
                snprintf(fileName, n, globalRadianceModelFileNameFormat, it);
                batchProcessFile(fileName, "w", batchSaveRadianceModel, scenePatches);
                delete[] fileName;
            }

            wasted_secs += (float) (wasted_start - clock()) / (float) CLOCKS_PER_SEC;

            fflush(stdout);
            fflush(stderr);

            it++;
            if ( globalIterations > 0 && it >= globalIterations ) {
                done = true;
            }
        }
    } else {
        printf("(No world-space radiance computations are being done)\n");
    }

    if ( globalTimings ) {
        fprintf(stdout, "Radiance total time %g secs.\n",
                ((float) (clock() - start_time) / (float) CLOCKS_PER_SEC) - wasted_secs);
    }

    if ( GLOBAL_raytracer_activeRaytracer ) {
        printf("Doing %s ...\n", GLOBAL_raytracer_activeRaytracer->fullName);

        start_time = clock();
        batchRayTrace(nullptr, nullptr, false, scenePatches, lightPatches);

        if ( globalTimings ) {
            fprintf(stdout, "Raytracing total time %g secs.\n",
                    (float) (clock() - start_time) / (float) CLOCKS_PER_SEC);
        }

        batchProcessFile(globalRaytracingImageFileName, "w", batchSaveRaytracingImage, scenePatches);
    } else {
        printf("(No pixel-based radiance computations are being done)\n");
    }

    printf("Computations finished.\n");
}
