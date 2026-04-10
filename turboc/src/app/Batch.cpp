#include <string.h>

#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "java/util/Formatter.h"
#include "common/RenderOptions.h"
#include "io/image/ImageOutputHandle.h"
#include "io/wrapper/FileUncompressWrapper.h"
#include "render/Canvas.h"
#include "render/RadianceImageExporter.h"
#include "app/Batch.h"
#include "app/options/OptionsGroupBatch.h"

#ifdef OPEN_GL_ENABLED
    #include "render/opengl/RenderOpenGL.h"
#endif

#ifdef RAYTRACING_ENABLED
    #include "app/Raytrace.h"
#endif

BatchOptions Batch::batchOptions;
const RayTracer *Batch::currentRayTracer = NULL;

const BatchOptions *
Batch::batchGetOptions() {
    return &batchOptions;
}

void
Batch::generalParseOptions(int *argc, char **argv) {
    OptionsGroupBatch::parse(argc, argv, batchOptions);
}

#ifdef RAYTRACING_ENABLED
void
Batch::batchRayTraceSaveImage(
    const char *fileName,
    OutputStream *outputStream,
    const int isPipe,
    const Scene *scene,
    const RadianceMethod * /*radianceMethod*/,
    const RayTracer *rayTracer,
    ToneMappingContext * /*toneMapOptions*/,
    const RenderOptions * /*renderOptions*/)
{
    Raytrace::rayTraceSaveImage(
        fileName,
        outputStream,
        isPipe,
        scene,
        rayTracer);
}
#endif

/**
This routine was copied from uit.c, leaving out all interface related things
*/
void
Batch::batchProcessFile(
    const char *fileName,
    void (*processFileCallback)(
        const char *fileName,
        OutputStream *outputStream,
        int isPipe,
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RayTracer *rayTracer,
        ToneMappingContext *toneMapOptions,
        const RenderOptions *renderOptions),
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RayTracer *rayTracer,
    ToneMappingContext *toneMapOptions,
    const RenderOptions *renderOptions)
{
    int isPipe;
    OutputStream *outputStream = FileUncompressWrapper::openOutputStreamCompressWrapper(fileName, &isPipe);

    // Call the user supplied procedure to process the file
    processFileCallback(fileName, outputStream, isPipe, scene, radianceMethod, rayTracer, toneMapOptions, renderOptions);

    FileUncompressWrapper::closeOutputStream(outputStream);
}

void
Batch::batchSaveRadianceImage(
    const char *fileName,
    OutputStream *outputStream,
    const int isPipe,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RayTracer * /*rayTracer*/,
    ToneMappingContext *toneMapOptions,
    const RenderOptions *renderOptions)
{
    long t;
    const char *extension;

    if ( outputStream == NULL ) {
        return;
    }

    Canvas::canvasPushMode();

    extension = ImageOutputHandle::imageFileExtension(fileName);
    if ( strncasecmp(extension, "logluv", 6) == 0 ) {
        System::out.printf("Saving LOGLUV image to file '%s' ....... ", fileName);
    } else {
        System::out.printf("Saving RGB image to file '%s' .......... ", fileName);
    }
    System::out.flush();

    t = System::nanoTime();

    RadianceImageExporter::exportImage(fileName, outputStream, isPipe, scene, radianceMethod, toneMapOptions, renderOptions);

    System::out.printf(
        "%g secs.\n",
        ((float)(((double)(System::nanoTime() - t)) / 1000000000.0)));
    Canvas::canvasPullMode();
}

void
Batch::batchSaveRadianceModel(
    const char *fileName,
    OutputStream *outputStream,
    const int /*isPipe*/,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RayTracer */*rayTracer*/,
    ToneMappingContext * /*toneMapOptions*/,
    const RenderOptions *renderOptions)
{
    long t;

    if ( outputStream == NULL ) {
        return;
    }

    Canvas::canvasPushMode();
    System::out.printf("Saving VRML model to file '%s' ... ", fileName);
    System::out.flush();
    t = System::nanoTime();

    if ( radianceMethod != NULL ) {
        radianceMethod->writeVRML(scene->camera, outputStream, renderOptions);
    }

    System::out.printf(
        "%g secs.\n",
        ((float)(((double)(System::nanoTime() - t)) / 1000000000.0)));
    Canvas::canvasPullMode();
}

void
Batch::batchExecuteRadianceSimulation(
    Scene *scene,
    RadianceMethod *radianceMethod,
    const RayTracer *rayTracer,
    ToneMappingContext *toneMapOptions,
    RenderOptions *renderOptions)
{
    long startTime;
    long wasted_start;
    float wastedSecs;

    if ( scene->geometryList == NULL || scene->geometryList->size() == 0 ) {
        System::out.printf("Empty world? Missing argument to some command line parameter option?\n");
        return;
    }

    Batch::currentRayTracer = rayTracer;

    startTime = System::nanoTime();
    wastedSecs = 0.0;

    if ( radianceMethod != NULL ) {
        System::out.printf("Doing %s ...\n", radianceMethod->getRadianceMethodName());

        System::out.flush();
        System::err.flush();

        bool done = false;
        for ( int iterationNumber = 0;
              iterationNumber < batchOptions.iterations && !done;
              iterationNumber++ ) {
            System::out.printf("-----------------------------------\n"
               "radiance iteration %04d\n"
               "-----------------------------------\n\n", iterationNumber);

            Canvas::canvasPushMode();
            done = radianceMethod->doStep(scene, renderOptions);
            Canvas::canvasPullMode();

            System::out.flush();
            System::err.flush();

            System::out.printf("%s", radianceMethod->getStats());

            #ifdef OPEN_GL_ENABLED
                RenderOpenGL::renderGetNearFar(scene->camera, scene->geometryList);
            #endif

            System::out.flush();
            System::err.flush();

            wasted_start = System::nanoTime();

            if ( (!(iterationNumber % batchOptions.saveModulo)) && *batchOptions.radianceImageFileNameFormat ) {
                int n = ((int)(strlen(batchOptions.radianceImageFileNameFormat))) + 1;
                char *fileName = new char[n];
                Formatter::format(
                    fileName,
                    n,
                    batchOptions.radianceImageFileNameFormat,
                    iterationNumber);
                Batch::batchProcessFile(
                    fileName,
                    Batch::batchSaveRadianceImage,
                    scene,
                    radianceMethod,
                    rayTracer,
                    toneMapOptions,
                    renderOptions);
                delete[] fileName;
            }

            if ( *batchOptions.radianceModelFileNameFormat ) {
                int n = ((int)(strlen(batchOptions.radianceModelFileNameFormat))) + 1;
                char *fileName = new char[n];
                Formatter::format(
                    fileName,
                    n,
                    batchOptions.radianceModelFileNameFormat,
                    iterationNumber);
                Batch::batchProcessFile(
                    fileName,
                    Batch::batchSaveRadianceModel,
                    scene,
                    radianceMethod,
                    rayTracer,
                    toneMapOptions,
                    renderOptions);
                delete[] fileName;
            }

            wastedSecs += ((float)(
                ((double)(wasted_start - System::nanoTime())) / 1000000000.0));

            System::out.flush();
            System::err.flush();
        }
    } else {
        System::out.printf("(No world-space radiance computations are being done)\n");
    }

    if ( batchOptions.timings ) {
        System::out.printf("Radiance total time %g secs.\n",
                ((float)(((double)(System::nanoTime() - startTime)) / 1000000000.0)) - wastedSecs);
    }

    #ifdef RAYTRACING_ENABLED
        if ( Batch::currentRayTracer != NULL ) {
            System::out.printf("Doing %s ...\n", Batch::currentRayTracer->getName());

            startTime = System::nanoTime();
            Raytrace::rayTraceExecute(
                NULL,
                NULL,
                false,
                scene,
                radianceMethod,
                Batch::currentRayTracer,
                toneMapOptions,
                renderOptions);

            if ( batchOptions.timings ) {
                System::out.printf("Raytracing total time %g secs.\n",
                        ((float)(((double)(System::nanoTime() - startTime)) / 1000000000.0)));
            }

            Batch::batchProcessFile(
                batchOptions.raytracingImageFileName,
                Batch::batchRayTraceSaveImage,
                scene,
                radianceMethod,
                Batch::currentRayTracer,
                toneMapOptions,
                renderOptions);
        } else {
            System::out.printf("(No pixel-based radiance computations are being done)\n");
        }
    #endif

    System::out.printf("Computations finished.\n");
    Batch::currentRayTracer = NULL;
}
