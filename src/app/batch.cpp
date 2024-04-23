#include <ctime>
#include <cstring>
#include <GL/gl.h>

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
    Camera *camera,
    Geometry *clusteredWorldGeometry,
    RadianceMethod *context)
{
    ImageOutputHandle *img;
    long j;
    long x = camera->xSize;
    long y = camera->ySize;
    GLubyte *screen;
    unsigned char *buf;

    // RayCast() saves the current picture in display-mapped (!) real values
    if ( GLOBAL_render_renderOptions.trace ) {
        rayCast(fileName, fp, isPipe, camera, clusteredWorldGeometry, context);
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
    void (*processFileCallback)(const char *fileName, FILE *fp, int isPipe, Camera *, java::ArrayList<Patch *> *scenePatches, Geometry *clusteredWorldGeometry, RadianceMethod *context),
    Camera *camera,
    java::ArrayList<Patch *> *scenePatches,
    Geometry *clusteredWorldGeometry,
    RadianceMethod *context)
{
    int isPipe;
    FILE *fp = openFileCompressWrapper(fileName, openMode, &isPipe);

    // Call the user supplied procedure to process the file
    processFileCallback(fileName, fp, isPipe, camera, scenePatches, clusteredWorldGeometry, context);

    closeFile(fp, isPipe);
}

static void
batchSaveRadianceImage(
    const char *fileName,
    FILE *fp,
    int isPipe,
    Camera *camera,
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

    openGlSaveScreen((char *)fileName, fp, isPipe, camera, clusteredWorldGeometry, context);

    fprintf(stdout, "%g secs.\n", (float) (clock() - t) / (float) CLOCKS_PER_SEC);
    canvasPullMode();
}

static void
batchSaveRadianceModel(
    const char *fileName,
    FILE *fp, int /*isPipe*/,
    Camera *camera,
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
        context->writeVRML(camera, fp);
    } else {
        writeVRML(camera, fp, scenePatches);
    }

    fprintf(stdout, "%g secs.\n", (float) (clock() - t) / (float) CLOCKS_PER_SEC);
    canvasPullMode();
}

void
batchParseOptions(int *argc, char **argv) {
    parseGeneralOptions(batchOptions, argc, argv);
}

void
batch(
    Camera *camera,
    Background *sceneBackground,
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Patch *> *lightPatches,
    java::ArrayList<Geometry *> *sceneGeometries,
    java::ArrayList<Geometry *> *sceneClusteredGeometries,
    Geometry *clusteredWorldGeometry,
    VoxelGrid *voxelGrid,
    RadianceMethod *radianceMethod)
{
    clock_t start_time, wasted_start;
    float wasted_secs;

    if ( sceneGeometries == nullptr || sceneGeometries->size() == 0 ) {
        printf("Empty world??\n");
        return;
    }

    start_time = clock();
    wasted_secs = 0.0;

    if ( radianceMethod != nullptr ) {
        // GLOBAL_scene_world-space radiance computations
        int it = 0;
        bool done = false;

        printf("Doing %s ...\n", radianceMethod->getRadianceMethodName());

        fflush(stdout);
        fflush(stderr);

        while ( !done ) {
            printf("-----------------------------------\n"
                   "GLOBAL_scene_world-space radiance iteration %04d\n"
                   "-----------------------------------\n\n", it);

            canvasPushMode();
            if ( radianceMethod == nullptr ) {
                fprintf(stderr, "No radiance method selected. Aborting program.\n");
                fflush(stderr);
                exit(1);
            }
            done = radianceMethod->doStep(
                camera,
                sceneBackground,
                scenePatches,
                sceneGeometries,
                sceneClusteredGeometries,
                lightPatches,
                clusteredWorldGeometry,
                voxelGrid);
            canvasPullMode();

            fflush(stdout);
            fflush(stderr);

            if ( radianceMethod != nullptr ) {
                printf("%s", radianceMethod->getStats());
            }

            int (*f)() = nullptr;
            #ifdef RAYTRACING_ENABLED
                if ( GLOBAL_raytracer_activeRaytracer != nullptr ) {
                    f = GLOBAL_raytracer_activeRaytracer->Redisplay;
                }
            #endif
            openGlRenderScene(
                    camera,
                    scenePatches,
                    sceneClusteredGeometries,
                    sceneGeometries,
                    clusteredWorldGeometry,
                    f,
                    radianceMethod);

            fflush(stdout);
            fflush(stderr);

            wasted_start = clock();

            if ( (!(it % globalSaveModulo)) && *globalRadianceImageFileNameFormat ) {
                int n = (int)strlen(globalRadianceImageFileNameFormat) + 1;
                char *fileName = new char[n];
                snprintf(fileName, n, globalRadianceImageFileNameFormat, it);
                batchProcessFile(
                        fileName,
                        "w",
                        batchSaveRadianceImage,
                        camera,
                        scenePatches,
                        clusteredWorldGeometry,
                        radianceMethod);
                delete[] fileName;
            }

            if ( *globalRadianceModelFileNameFormat ) {
                int n = (int)strlen(globalRadianceModelFileNameFormat) + 1;
                char *fileName = new char[n];
                snprintf(fileName, n, globalRadianceModelFileNameFormat, it);
                batchProcessFile(
                        fileName,
                        "w",
                        batchSaveRadianceModel,
                        camera,
                        scenePatches,
                        clusteredWorldGeometry,
                        radianceMethod);
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
            batchRayTrace(
                    nullptr,
                    nullptr,
                    false,
                    camera,
                    sceneBackground,
                    voxelGrid,
                    scenePatches,
                    lightPatches,
                    clusteredWorldGeometry,
                    radianceMethod);

            if ( globalTimings ) {
                fprintf(stdout, "Raytracing total time %g secs.\n",
                        (float) (clock() - start_time) / (float) CLOCKS_PER_SEC);
            }

            batchProcessFile(
                    globalRaytracingImageFileName,
                    "w",
                    batchSaveRaytracingImage,
                    camera,
                    scenePatches,
                    clusteredWorldGeometry,
                    radianceMethod);
        } else {
            printf("(No pixel-based radiance computations are being done)\n");
        }
    #endif

    printf("Computations finished.\n");
}
