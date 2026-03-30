#include <cstring>

#ifdef OPEN_GL_ENABLED
    #ifdef __APPLE__
        #include <OpenGL/gl.h>
    #else
        #include <GL/gl.h>
    #endif
#endif

#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "java/util/Formatter.h"
#include "common/RenderOptions.h"
#include "io/wrapper/FileUncompressWrapper.h"
#include "render/Canvas.h"
#include "render/Render.h"
#include "raycasting/simple/RayCaster.h"
#include "app/Batch.h"
#include "app/CommandLine.h"

#ifdef RAYTRACING_ENABLED
    #include "app/Raytrace.h"
#endif

static BatchOptions globalBatchOptions;

const BatchOptions *
Batch::batchGetOptions() {
    return &globalBatchOptions;
}

void
Batch::generalParseOptions(int *argc, char **argv) {
    CommandLine::batchParseOptions(argc, argv, &globalBatchOptions);
}

/**
Saves a RGB image in the front buffer
*/
void
Batch::openGlSaveScreen(
    const char *fileName,
    java::io::OutputStream *outputStream,
    const int isPipe,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions)
{
    // RayCast() saves the current picture in display-mapped (!) real values
    if ( renderOptions->trace ) {
        RayCaster::rayCast(fileName, outputStream, isPipe, scene, radianceMethod, renderOptions);
        return;
    }

    long x = scene->camera->xSize;
    long y = scene->camera->ySize;
    ImageOutputHandle *image = ImageOutputHandle::createImageOutputHandle(fileName, outputStream, isPipe, static_cast<int>(x), static_cast<int>(y));
    if ( image == nullptr ) {
        return;
    }

    unsigned char *screen = new unsigned char[x * y * 4];
    unsigned char *buffer = new unsigned char[3 * x];

#ifdef OPEN_GL_ENABLED
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, static_cast<int>(x), static_cast<int>(y), GL_RGBA, GL_UNSIGNED_BYTE, screen);
#endif

    for ( long j = y - 1; j >= 0; j-- ) {
        const long screenRowStart = j * x * 4;
        for ( long i = 0; i < x; i++ ) {
            const long pixelOffset = screenRowStart + i * 4;
            const long bufferOffset = i * 3;
            buffer[bufferOffset] = screen[pixelOffset];
            buffer[bufferOffset + 1] = screen[pixelOffset + 1];
            buffer[bufferOffset + 2] = screen[pixelOffset + 2];
        }
        ImageOutputHandle::writeDisplayRGB(image, buffer);
    }

    delete[] buffer;
    delete[] screen;
    delete image;
}

#ifdef RAYTRACING_ENABLED
void
Batch::batchRayTraceSaveImage(
    const char *fileName,
    java::io::OutputStream *outputStream,
    const int isPipe,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RayTracer *rayTracer,
    const RenderOptions *renderOptions)
{
    Raytrace::rayTraceSaveImage(
        fileName,
        outputStream,
        isPipe,
        scene,
        radianceMethod,
        rayTracer,
        renderOptions);
}
#endif

/**
This routine was copied from uit.c, leaving out all interface related things
*/
void
Batch::batchProcessFile(
    const char *fileName,
    void (*processFileCallback)(const char *fileName, java::io::OutputStream *outputStream, int isPipe, const Scene *scene, const RadianceMethod *radianceMethod, const RayTracer *rayTracer, const RenderOptions *renderOptions),
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RayTracer *rayTracer,
    const RenderOptions *renderOptions)
{
    int isPipe;
    java::io::OutputStream *outputStream = FileUncompressWrapper::openOutputStreamCompressWrapper(fileName, &isPipe);

    // Call the user supplied procedure to process the file
    processFileCallback(fileName, outputStream, isPipe, scene, radianceMethod, rayTracer, renderOptions);

    FileUncompressWrapper::closeOutputStream(outputStream);
}

void
Batch::batchSaveRadianceImage(
    const char *fileName,
    java::io::OutputStream *outputStream,
    const int isPipe,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RayTracer * /*rayTracer*/,
    const RenderOptions *renderOptions)
{
    long long t;
    const char *extension;

    if ( outputStream == nullptr ) {
        return;
    }

    Canvas::canvasPushMode();

    extension = ImageOutputHandle::imageFileExtension(fileName);
    if ( strncasecmp(extension, "logluv", 6) == 0 ) {
        java::lang::System::out.printf("Saving LOGLUV image to file '%s' ....... ", fileName);
    } else {
        java::lang::System::out.printf("Saving RGB image to file '%s' .......... ", fileName);
    }
    java::lang::System::out.flush();

    t = java::lang::System::nanoTime();

    // No OpenGL really if renderOptions->trace is true
    Batch::openGlSaveScreen(fileName, outputStream, isPipe, scene, radianceMethod, renderOptions);

    java::lang::System::out.printf(
        "%g secs.\n",
        static_cast<float>(static_cast<double>(java::lang::System::nanoTime() - t) / 1000000000.0));
    Canvas::canvasPullMode();
}

void
Batch::batchSaveRadianceModel(
    const char *fileName,
    java::io::OutputStream *outputStream,
    const int /*isPipe*/,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RayTracer */*rayTracer*/,
    const RenderOptions *renderOptions)
{
    long long t;

    if ( outputStream == nullptr ) {
        return;
    }

    Canvas::canvasPushMode();
    java::lang::System::out.printf("Saving VRML model to file '%s' ... ", fileName);
    java::lang::System::out.flush();
    t = java::lang::System::nanoTime();

    if ( radianceMethod != nullptr ) {
        radianceMethod->writeVRML(scene->camera, outputStream, renderOptions);
    }

    java::lang::System::out.printf(
        "%g secs.\n",
        static_cast<float>(static_cast<double>(java::lang::System::nanoTime() - t) / 1000000000.0));
    Canvas::canvasPullMode();
}

void
Batch::batchExecuteRadianceSimulation(
    Scene *scene,
    RadianceMethod *radianceMethod,
    const RayTracer *rayTracer,
    RenderOptions *renderOptions)
{
    long long startTime;
    long long wasted_start;
    float wastedSecs;

    if ( scene->geometryList == nullptr || scene->geometryList->size() == 0 ) {
        java::lang::System::out.printf("Empty world? Missing argument to some command line parameter option?\n");
        return;
    }

    startTime = java::lang::System::nanoTime();
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

            Canvas::canvasPushMode();
            done = radianceMethod->doStep(scene, renderOptions);
            Canvas::canvasPullMode();

            java::lang::System::out.flush();
            java::lang::System::err.flush();

            java::lang::System::out.printf("%s", radianceMethod->getStats());

            Render::renderGetNearFar(scene->camera, scene->geometryList);

            java::lang::System::out.flush();
            java::lang::System::err.flush();

            wasted_start = java::lang::System::nanoTime();

            if ( (!(iterationNumber % globalBatchOptions.saveModulo)) && *globalBatchOptions.radianceImageFileNameFormat ) {
                int n = static_cast<int>(strlen(globalBatchOptions.radianceImageFileNameFormat)) + 1;
                char *fileName = new char[n];
                java::util::Formatter::format(
                    fileName,
                    n,
                    globalBatchOptions.radianceImageFileNameFormat,
                    iterationNumber);
                Batch::batchProcessFile(
                    fileName,
                    Batch::batchSaveRadianceImage,
                    scene,
                    radianceMethod,
                    rayTracer,
                    renderOptions);
                delete[] fileName;
            }

            if ( *globalBatchOptions.radianceModelFileNameFormat ) {
                int n = static_cast<int>(strlen(globalBatchOptions.radianceModelFileNameFormat)) + 1;
                char *fileName = new char[n];
                java::util::Formatter::format(
                    fileName,
                    n,
                    globalBatchOptions.radianceModelFileNameFormat,
                    iterationNumber);
                Batch::batchProcessFile(
                    fileName,
                    Batch::batchSaveRadianceModel,
                    scene,
                    radianceMethod,
                    rayTracer,
                    renderOptions);
                delete[] fileName;
            }

            wastedSecs += static_cast<float>(
                static_cast<double>(wasted_start - java::lang::System::nanoTime()) / 1000000000.0);

            java::lang::System::out.flush();
            java::lang::System::err.flush();
        }
    } else {
        java::lang::System::out.printf("(No world-space radiance computations are being done)\n");
    }

    if ( globalBatchOptions.timings ) {
        java::lang::System::out.printf("Radiance total time %g secs.\n",
                static_cast<float>(static_cast<double>(java::lang::System::nanoTime() - startTime) / 1000000000.0) - wastedSecs);
    }

    #ifdef RAYTRACING_ENABLED
        if ( GLOBAL_rayTracer != nullptr ) {
            java::lang::System::out.printf("Doing %s ...\n", rayTracer->getName());

            startTime = java::lang::System::nanoTime();
            Raytrace::rayTraceExecute(
                nullptr,
                nullptr,
                false,
                scene,
                radianceMethod,
                rayTracer,
                renderOptions);

            if ( globalBatchOptions.timings ) {
                java::lang::System::out.printf("Raytracing total time %g secs.\n",
                        static_cast<float>(static_cast<double>(java::lang::System::nanoTime() - startTime) / 1000000000.0));
            }

            Batch::batchProcessFile(
                globalBatchOptions.raytracingImageFileName,
                Batch::batchRayTraceSaveImage,
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
