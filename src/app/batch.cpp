#include <ctime>
#include <cstring>
#include "java/lang/System.h"
#include <GL/gl.h>

#include "common/RenderOptions.h"
#include "java/util/ArrayList.txx"
#include "render/canvas.h"
#include "render/render.h"
#include "io/FileUncompressWrapper.h"
#include "raycasting/simple/RayCaster.h"
#include "app/commandLine.h"
#include "app/BatchOptions.h"
#include "app/batch.h"

#ifdef RAYTRACING_ENABLED
    #include "raycasting/common/Raytracer.h"
    #include "app/raytrace.h"
    #include "render/opengl.h"
#endif

static BatchOptions globalBatchOptions;

const BatchOptions *
batchGetOptions() {
    return &globalBatchOptions;
}

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
    ImageOutputHandle *image = createImageOutputHandle(fileName, fp, isPipe, static_cast<int>(x), static_cast<int>(y));
    if ( image == nullptr ) {
        return;
    }

    GLubyte *screen = new GLubyte[x * y * 4];
    unsigned char *buffer = new unsigned char[3 * x];

#ifdef OPEN_GL_ENABLED
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, static_cast<int>(x), static_cast<int>(y), GL_RGBA, GL_UNSIGNED_BYTE, screen);
#endif

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
    void (*processFileCallback)(const char *fileName, FILE *fp, int isPipe, const Scene *scene, const RadianceMethod *radianceMethod, const RayTracer *rayTracer, const RenderOptions *renderOptions),
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RayTracer *rayTracer,
    const RenderOptions *renderOptions)
{
    int isPipe;
    FILE *fp = openFileCompressWrapper(fileName, openMode, &isPipe);

    // Call the user supplied procedure to process the file
    processFileCallback(fileName, fp, isPipe, scene, radianceMethod, rayTracer, renderOptions);

    closeFile(fp, isPipe);
}

static void
batchSaveRadianceImage(
    const char *fileName,
    FILE *fp,
    const int isPipe,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RayTracer * /*rayTracer*/,
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
        java::lang::System::out.printf("Saving LOGLUV image to file '%s' ....... ", fileName);
    } else {
        java::lang::System::out.printf("Saving RGB image to file '%s' .......... ", fileName);
    }
    java::lang::System::out.flush();

    t = clock();

    // No OpenGL really if renderOptions->trace is true
    openGlSaveScreen(fileName, fp, isPipe, scene, radianceMethod, renderOptions);

    java::lang::System::out.printf("%g secs.\n", static_cast<float>(clock() - t) / static_cast<float>(CLOCKS_PER_SEC));
    canvasPullMode();
}

static void
batchSaveRadianceModel(
    const char *fileName,
    FILE *fp,
    int /*isPipe*/,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RayTracer */*rayTracer*/,
    const RenderOptions *renderOptions)
{
    clock_t t;

    if ( !fp ) {
        return;
    }

    canvasPushMode();
    java::lang::System::out.printf("Saving VRML model to file '%s' ... ", fileName);
    java::lang::System::out.flush();
    t = clock();

    if ( radianceMethod != nullptr ) {
        radianceMethod->writeVRML(scene->camera, fp, renderOptions);
    }

    java::lang::System::out.printf("%g secs.\n", static_cast<float>(clock() - t) / static_cast<float>(CLOCKS_PER_SEC));
    canvasPullMode();
}

void
batchExecuteRadianceSimulation(
    Scene *scene,
    RadianceMethod *radianceMethod,
    const RayTracer *rayTracer,
    RenderOptions *renderOptions)
{
    clock_t startTime;
    clock_t wasted_start;
    float wastedSecs;

    if ( scene->geometryList == nullptr || scene->geometryList->size() == 0 ) {
        java::lang::System::out.printf("Empty world? Missing argument to some command line parameter option?\n");
        return;
    }

    startTime = clock();
    wastedSecs = 0.0;

    if ( radianceMethod != nullptr ) {
        java::lang::System::out.printf("Doing %s ...\n", radianceMethod->getRadianceMethodName());

        java::lang::System::out.flush();
        java::lang::System::err.flush();

        bool done = false;
        for ( int iterationNumber = 0;
              iterationNumber < globalBatchOptions.iterations && !done;
              iterationNumber++ ) {
            java::lang::System::out.printf("-----------------------------------\n"
                   "GLOBAL_scene_world-space radiance iteration %04d\n"
                   "-----------------------------------\n\n", iterationNumber);

            canvasPushMode();
            done = radianceMethod->doStep(scene, renderOptions);
            canvasPullMode();

            java::lang::System::out.flush();
            java::lang::System::err.flush();

            java::lang::System::out.printf("%s", radianceMethod->getStats());

            renderGetNearFar(scene->camera, scene->geometryList);

            java::lang::System::out.flush();
            java::lang::System::err.flush();

            wasted_start = clock();

            if ( (!(iterationNumber % globalBatchOptions.saveModulo)) && *globalBatchOptions.radianceImageFileNameFormat ) {
                int n = static_cast<int>(strlen(globalBatchOptions.radianceImageFileNameFormat)) + 1;
                char *fileName = new char[n];
                snprintf(fileName, n, globalBatchOptions.radianceImageFileNameFormat, iterationNumber);
                batchProcessFile(
                    fileName,
                    "w",
                    batchSaveRadianceImage,
                    scene,
                    radianceMethod,
                    rayTracer,
                    renderOptions);
                delete[] fileName;
            }

            if ( *globalBatchOptions.radianceModelFileNameFormat ) {
                int n = static_cast<int>(strlen(globalBatchOptions.radianceModelFileNameFormat)) + 1;
                char *fileName = new char[n];
                snprintf(fileName, n, globalBatchOptions.radianceModelFileNameFormat, iterationNumber);
                batchProcessFile(
                    fileName,
                    "w",
                    batchSaveRadianceModel,
                    scene,
                    radianceMethod,
                    rayTracer,
                    renderOptions);
                delete[] fileName;
            }

            wastedSecs += static_cast<float>(wasted_start - clock()) / static_cast<float>(CLOCKS_PER_SEC);

            java::lang::System::out.flush();
            java::lang::System::err.flush();
        }
    } else {
        java::lang::System::out.printf("(No world-space radiance computations are being done)\n");
    }

    if ( globalBatchOptions.timings ) {
        java::lang::System::out.printf("Radiance total time %g secs.\n",
                (static_cast<float>(clock() - startTime) / static_cast<float>(CLOCKS_PER_SEC)) - wastedSecs);
    }

    #ifdef RAYTRACING_ENABLED
        if ( GLOBAL_rayTracer != nullptr ) {
            java::lang::System::out.printf("Doing %s ...\n", rayTracer->getName());

            startTime = clock();
            rayTraceExecute(
                nullptr,
                nullptr,
                false,
                scene,
                radianceMethod,
                rayTracer,
                renderOptions);

            if ( globalBatchOptions.timings ) {
                java::lang::System::out.printf("Raytracing total time %g secs.\n",
                        static_cast<float>(clock() - startTime) / static_cast<float>(CLOCKS_PER_SEC));
            }

            batchProcessFile(
                globalBatchOptions.raytracingImageFileName,
                "w",
                rayTraceSaveImage,
                scene,
                radianceMethod,
                rayTracer,
                renderOptions);
        } else {
            java::lang::System::out.printf("(No pixel-based radiance computations are being done)\n");
        }
    #endif

    java::lang::System::out.printf("Computations finished.\n");
}
