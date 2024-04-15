#include <ctime>
#include <cstring>
#include <GL/gl.h>

#include "scene/scene.h"
#include "io/writevrml.h"
#include "common/options.h"
#include "render/canvas.h"
#include "io/FileUncompressWrapper.h"
#include "raycasting/simple/RayCaster.h"
#include "render/opengl.h"
#include "IMAGE/imagec.h"
#include "app/batch.h"

#ifdef RAYTRACING_ENABLED
    #include "raycasting/common/Raytracer.h"
    #include "app/raytrace.h"
#endif

static int globalIterations = 1; // Radiance method iterations
static int globalSaveModulo = 10; // Every 10th iteration, surface model and image will be saved
static int globalTimings = false;
static const char *globalRadianceImageFileNameFormat = "";
static const char *globalRadianceModelFileNameFormat = "";
static const char *globalRaytracingImageFileName = "";

static CommandLineOptionDescription batchOptions[] = {
    {"-iterations", 3, &GLOBAL_options_intType, &globalIterations, DEFAULT_ACTION,
     "-iterations <integer>\t: world-space radiance iterations"},
    {"-radiance-image-savefile", 12, Tstring,
     &globalRadianceImageFileNameFormat, DEFAULT_ACTION,
     "-radiance-image-savefile <filename>\t: radiance PPM/LOGLUV savefile name,"
     "\n\tfirst '%%d' will be substituted by iteration number"},
    {"-radiance-model-savefile", 12, Tstring,
     &globalRadianceModelFileNameFormat, DEFAULT_ACTION,
     "-radiance-model-savefile <filename>\t: radiance VRML model savefile name,"
     "\n\tfirst '%%d' will be substituted by iteration number"},
    {"-save-modulo", 8, &GLOBAL_options_intType, &globalSaveModulo, DEFAULT_ACTION,
     "-save-modulo <integer>\t: save every n-th iteration"},
    {"-raytracing-image-savefile", 14, Tstring, &globalRaytracingImageFileName, DEFAULT_ACTION,
     "-raytracing-image-savefile <filename>\t: raytracing PPM savefile name"},
    {"-timings", 3, Tsettrue, &globalTimings, DEFAULT_ACTION,
     "-timings\t: printRegularHierarchy timings for world-space radiance and raytracing methods"},
    {nullptr, 0,  TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

/**
Saves a RGB image in the front buffer
*/
static void
openGlSaveScreen(
    char *fileName,
    FILE *fp,
    int isPipe,
    Geometry *clusteredWorldGeometry,
    RadianceMethod *context)
{
    ImageOutputHandle *img;
    long j, x = GLOBAL_camera_mainCamera.xSize, y = GLOBAL_camera_mainCamera.ySize;
    GLubyte *screen;
    unsigned char *buf;

    // RayCast() saves the current picture in display-mapped (!) real values
    if ( GLOBAL_render_renderOptions.trace ) {
        rayCast(fileName, fp, isPipe, clusteredWorldGeometry, context);
        return;
    }

    if ( !(img = createImageOutputHandle(fileName, fp, isPipe, (int)x, (int)y)) ) {
        return;
    }

    screen = (GLubyte *)malloc((int) (x * y) * sizeof(GLubyte) * 4);
    buf = (unsigned char *)malloc(3 * x);

    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, (int)x, (int)y, GL_RGBA, GL_UNSIGNED_BYTE, screen);

    for ( j = y - 1; j >= 0; j-- ) {
        long i;
        unsigned char *bufferPosition = buf;
        GLubyte *pixel = &screen[j * x * 4];
        for ( i = 0; i < x; i++, pixel += 4 ) {
            *bufferPosition++ = pixel[0];
            *bufferPosition++ = pixel[1];
            *bufferPosition++ = pixel[2];
        }
        writeDisplayRGB(img, buf);
    }

    free(buf);
    free(screen);

    deleteImageOutputHandle(img);
}

/**
This routine was copied from uit.c, leaving out all interface related things
*/
static void
batchProcessFile(
    const char *fileName,
    const char *openMode,
    void (*processFileCallback)(const char *fileName, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches, Geometry *clusteredWorldGeometry, RadianceMethod *context),
    java::ArrayList<Patch *> *scenePatches,
    Geometry *clusteredWorldGeometry,
    RadianceMethod *context)
{
    int isPipe;
    FILE *fp = openFileCompressWrapper(fileName, openMode, &isPipe);

    // Call the user supplied procedure to process the file
    processFileCallback(fileName, fp, isPipe, scenePatches, clusteredWorldGeometry, context);

    closeFile(fp, isPipe);
}

static void
batchSaveRadianceImage(
    const char *fileName,
    FILE *fp,
    int isPipe,
    java::ArrayList<Patch *> * /*scenePatches*/,
    Geometry *clusteredWorldGeometry,
    RadianceMethod *context)
{
    clock_t t;
    char *extension;

    if ( !fp ) {
        return;
    }

    canvasPushMode();

    extension = imageFileExtension((char *) fileName);
    if ( IS_TIFF_LOGLUV_EXT(extension) ) {
        fprintf(stdout, "Saving LOGLUV image to file '%s' ....... ", fileName);
    } else {
        fprintf(stdout, "Saving RGB image to file '%s' .......... ", fileName);
    }
    fflush(stdout);

    t = clock();

    openGlSaveScreen((char *)fileName, fp, isPipe, clusteredWorldGeometry, context);

    fprintf(stdout, "%g secs.\n", (float) (clock() - t) / (float) CLOCKS_PER_SEC);
    canvasPullMode();
}

static void
batchSaveRadianceModel(
    const char *fileName,
    FILE *fp, int /*isPipe*/,
    java::ArrayList<Patch *> *scenePatches,
    Geometry *clusteredWorldGeometry,
    RadianceMethod *context)
{
    clock_t t;

    if ( !fp ) {
        return;
    }

    canvasPushMode();
    fprintf(stdout, "Saving VRML model to file '%s' ... ", fileName);
    fflush(stdout);
    t = clock();

    if ( context != nullptr ) {
        context->writeVRML(fp);
    } else {
        writeVRML(fp, scenePatches);
    }

    fprintf(stdout, "%g secs.\n", (float) (clock() - t) / (float) CLOCKS_PER_SEC);
    canvasPullMode();
}

void
parseBatchOptions(int *argc, char **argv) {
    parseGeneralOptions(batchOptions, argc, argv);
}

void
batch(
    Background *sceneBackground,
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Patch *> *lightPatches,
    java::ArrayList<Geometry *> *sceneGeometries,
    Geometry *clusteredWorldGeometry,
    VoxelGrid *voxelGrid,
    RadianceMethod *context)
{
    clock_t start_time, wasted_start;
    float wasted_secs;

    if ( sceneGeometries == nullptr || sceneGeometries->size() == 0 ) {
        printf("Empty world??\n");
        return;
    }

    start_time = clock();
    wasted_secs = 0.0;

    if ( context != nullptr ) {
        // GLOBAL_scene_world-space radiance computations
        int it = 0;
        bool done = false;

        printf("Doing %s ...\n", context->getRadianceMethodName());

        fflush(stdout);
        fflush(stderr);

        while ( !done ) {
            printf("-----------------------------------\n"
                   "GLOBAL_scene_world-space radiance iteration %04d\n"
                   "-----------------------------------\n\n", it);

            canvasPushMode();
            if ( context == nullptr ) {
                fprintf(stderr, "No radiance method selected. Aborting program.\n");
                fflush(stderr);
                exit(1);
            }
            done = context->doStep(
                sceneBackground,
                scenePatches,
                sceneGeometries,
                lightPatches,
                clusteredWorldGeometry,
                voxelGrid);
            canvasPullMode();

            fflush(stdout);
            fflush(stderr);

            if ( context != nullptr ) {
                printf("%s", context->getStats());
            }

            int (*f)() = nullptr;
            #ifdef RAYTRACING_ENABLED
                if ( GLOBAL_raytracer_activeRaytracer != nullptr ) {
                    f = GLOBAL_raytracer_activeRaytracer->Redisplay;
                }
            #endif
            openGlRenderScene(
                scenePatches,
                GLOBAL_scene_clusteredGeometries,
                sceneGeometries,
                clusteredWorldGeometry,
                f,
                context);

            fflush(stdout);
            fflush(stderr);

            wasted_start = clock();

            if ( (!(it % globalSaveModulo)) && *globalRadianceImageFileNameFormat ) {
                int n = (int)strlen(globalRadianceImageFileNameFormat) + 1;
                char *fileName = new char[n];
                snprintf(fileName, n, globalRadianceImageFileNameFormat, it);
                batchProcessFile(fileName, "w", batchSaveRadianceImage, scenePatches, clusteredWorldGeometry, context);
                delete[] fileName;
            }

            if ( *globalRadianceModelFileNameFormat ) {
                int n = (int)strlen(globalRadianceModelFileNameFormat) + 1;
                char *fileName = new char[n];
                snprintf(fileName, n, globalRadianceModelFileNameFormat, it);
                batchProcessFile(fileName, "w", batchSaveRadianceModel, scenePatches, clusteredWorldGeometry, context);
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

    #ifdef RAYTRACING_ENABLED
        if ( GLOBAL_raytracer_activeRaytracer ) {
            printf("Doing %s ...\n", GLOBAL_raytracer_activeRaytracer->fullName);

            start_time = clock();
            batchRayTrace(nullptr, nullptr, false, sceneBackground, scenePatches, lightPatches, clusteredWorldGeometry, context);

            if ( globalTimings ) {
                fprintf(stdout, "Raytracing total time %g secs.\n",
                        (float) (clock() - start_time) / (float) CLOCKS_PER_SEC);
            }

            batchProcessFile(
                globalRaytracingImageFileName,
                "w",
                batchSaveRaytracingImage,
                scenePatches,
                clusteredWorldGeometry,
                context);
        } else {
            printf("(No pixel-based radiance computations are being done)\n");
        }
    #endif

    printf("Computations finished.\n");
}
