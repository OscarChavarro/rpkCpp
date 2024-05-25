#include <ctime>
#include <cstring>
#include <GL/gl.h>

#include "java/util/ArrayList.txx"
#include "io/writevrml.h"
#include "render/canvas.h"
#include "io/FileUncompressWrapper.h"
#include "raycasting/simple/RayCaster.h"
#include "render/opengl.h"
#include "IMAGE/imagec.h"
#include "app/options.h"
#include "app/commandLine.h"
#include "app/BatchOptions.h"
#include "app/batch.h"

#ifdef RAYTRACING_ENABLED
    #include "raycasting/common/Raytracer.h"
    #include "app/raytrace.h"
#endif

static BatchOptions globalBatchOptions;

void
generalParseOptions(int *argc, char **argv) {
    batchParseOptions(argc, argv, &globalBatchOptions);
}

/**
Saves a RGB image in the front buffer
*/
static void
openGlSaveScreen(
    const char *fileName,
    FILE *fp,
    const int isPipe,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions)
{
    // RayCast() saves the current picture in display-mapped (!) real values
    if ( renderOptions->trace ) {
        rayCast(fileName, fp, isPipe, scene, radianceMethod, renderOptions);
        return;
    }

    long x = scene->camera->xSize;
    long y = scene->camera->ySize;
    ImageOutputHandle *image = createImageOutputHandle(fileName, fp, isPipe, (int)x, (int)y);
    if ( image == nullptr ) {
        return;
    }

    GLubyte *screen = new GLubyte[x * y * 4];
    unsigned char *buffer = new unsigned char[3 * x];

    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, (int)x, (int)y, GL_RGBA, GL_UNSIGNED_BYTE, screen);

    for ( long j = y - 1; j >= 0; j-- ) {
        unsigned char *bufferPosition = buffer;
        const GLubyte *pixel = &screen[j * x * 4];
        for ( long i = 0; i < x; i++, pixel += 4 ) {
            *bufferPosition++ = pixel[0];
            *bufferPosition++ = pixel[1];
            *bufferPosition++ = pixel[2];
        }
        writeDisplayRGB(image, buffer);
    }

    delete[] buffer;
    delete[] screen;
    delete image;
}

/**
This routine was copied from uit.c, leaving out all interface related things
*/
static void
batchProcessFile(
    const char *fileName,
    const char *openMode,
    void (*processFileCallback)(const char *fileName, FILE *fp, int isPipe, const Scene *scene, const RadianceMethod *radianceMethod, const RenderOptions *renderOptions),
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions)
{
    int isPipe;
    FILE *fp = openFileCompressWrapper(fileName, openMode, &isPipe);

    // Call the user supplied procedure to process the file
    processFileCallback(fileName, fp, isPipe, scene, radianceMethod, renderOptions);

    closeFile(fp, isPipe);
}

static void
batchSaveRadianceImage(
    const char *fileName,
    FILE *fp,
    const int isPipe,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions)
{
    clock_t t;
    const char *extension;

    if ( !fp ) {
        return;
    }

    canvasPushMode();

    extension = imageFileExtension(fileName);
    if ( strncasecmp(extension, "logluv", 6) == 0 ) {
        fprintf(stdout, "Saving LOGLUV image to file '%s' ....... ", fileName);
    } else {
        fprintf(stdout, "Saving RGB image to file '%s' .......... ", fileName);
    }
    fflush(stdout);

    t = clock();

    openGlSaveScreen(fileName, fp, isPipe, scene, radianceMethod, renderOptions);

    fprintf(stdout, "%g secs.\n", (float) (clock() - t) / (float) CLOCKS_PER_SEC);
    canvasPullMode();
}

static void
batchSaveRadianceModel(
    const char *fileName,
    FILE *fp,
    int /*isPipe*/,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions)
{
    clock_t t;

    if ( !fp ) {
        return;
    }

    canvasPushMode();
    fprintf(stdout, "Saving VRML model to file '%s' ... ", fileName);
    fflush(stdout);
    t = clock();

    if ( radianceMethod != nullptr ) {
        radianceMethod->writeVRML(scene->camera, fp, renderOptions);
    } else {
        writeVRML(scene->camera, fp, scene->patchList);
    }

    fprintf(stdout, "%g secs.\n", (float) (clock() - t) / (float) CLOCKS_PER_SEC);
    canvasPullMode();
}

void
batchExecuteRadianceSimulation(Scene *scene, RadianceMethod *radianceMethod, RenderOptions *renderOptions) {
    clock_t start_time;
    clock_t wasted_start;
    float wastedSecs;

    if ( scene->geometryList == nullptr || scene->geometryList->size() == 0 ) {
        printf("Empty world??\n");
        return;
    }

    start_time = clock();
    wastedSecs = 0.0;

    if ( radianceMethod != nullptr ) {
        printf("Doing %s ...\n", radianceMethod->getRadianceMethodName());

        fflush(stdout);
        fflush(stderr);

        bool done = false;
        for ( int iterationNumber = 0;
              iterationNumber < globalBatchOptions.iterations && !done;
              iterationNumber++ ) {
            printf("-----------------------------------\n"
                   "GLOBAL_scene_world-space radiance iteration %04d\n"
                   "-----------------------------------\n\n", iterationNumber);

            canvasPushMode();
            done = radianceMethod->doStep(scene, renderOptions);
            canvasPullMode();

            fflush(stdout);
            fflush(stderr);

            printf("%s", radianceMethod->getStats());

            int (*f)() = nullptr;
            #ifdef RAYTRACING_ENABLED
                if ( GLOBAL_raytracer_activeRaytracer != nullptr ) {
                    f = GLOBAL_raytracer_activeRaytracer->Redisplay;
                }
            #endif
            openGlRenderScene(scene, f, radianceMethod, renderOptions);

            fflush(stdout);
            fflush(stderr);

            wasted_start = clock();

            if ( (!(iterationNumber % globalBatchOptions.saveModulo)) && *globalBatchOptions.radianceImageFileNameFormat ) {
                int n = (int)strlen(globalBatchOptions.radianceImageFileNameFormat) + 1;
                char *fileName = new char[n];
                snprintf(fileName, n, globalBatchOptions.radianceImageFileNameFormat, iterationNumber);
                batchProcessFile(
                    fileName,
                    "w",
                    batchSaveRadianceImage,
                    scene,
                    radianceMethod,
                    renderOptions);
                delete[] fileName;
            }

            if ( *globalBatchOptions.radianceModelFileNameFormat ) {
                int n = (int)strlen(globalBatchOptions.radianceModelFileNameFormat) + 1;
                char *fileName = new char[n];
                snprintf(fileName, n, globalBatchOptions.radianceModelFileNameFormat, iterationNumber);
                batchProcessFile(
                    fileName,
                    "w",
                    batchSaveRadianceModel,
                    scene,
                    radianceMethod,
                    renderOptions);
                delete[] fileName;
            }

            wastedSecs += (float) (wasted_start - clock()) / (float) CLOCKS_PER_SEC;

            fflush(stdout);
            fflush(stderr);
        }
    } else {
        printf("(No world-space radiance computations are being done)\n");
    }

    if ( globalBatchOptions.timings ) {
        fprintf(stdout, "Radiance total time %g secs.\n",
                ((float) (clock() - start_time) / (float) CLOCKS_PER_SEC) - wastedSecs);
    }

    #ifdef RAYTRACING_ENABLED
        if ( GLOBAL_raytracer_activeRaytracer ) {
            printf("Doing %s ...\n", GLOBAL_raytracer_activeRaytracer->fullName);

            start_time = clock();
            rayTraceExecute(
                    nullptr,
                    nullptr,
                    false,
                    scene,
                    radianceMethod,
                    renderOptions);

            if ( globalBatchOptions.timings ) {
                fprintf(stdout, "Raytracing total time %g secs.\n",
                        (float) (clock() - start_time) / (float) CLOCKS_PER_SEC);
            }

            batchProcessFile(
                    globalBatchOptions.raytracingImageFileName,
                    "w",
                    rayTraceSaveImage,
                    scene,
                    radianceMethod,
                    renderOptions);
        } else {
            printf("(No pixel-based radiance computations are being done)\n");
        }
    #endif

    printf("Computations finished.\n");
}
