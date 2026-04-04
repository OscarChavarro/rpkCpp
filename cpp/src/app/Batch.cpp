#include <cstring>

#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "java/util/Formatter.h"
#include "common/RenderOptions.h"
#include "io/image/ImageOutputHandle.h"
#include "io/wrapper/FileUncompressWrapper.h"
#include "render/Canvas.h"
#include "render/RadianceImageExporter.h"
#include "app/Batch.h"
#include "app/options/BatchOptionsParser.h"

#ifdef OPEN_GL_ENABLED
    #include "render/opengl/RenderOpenGL.h"
#endif

#ifdef RAYTRACING_ENABLED
    #include "app/Raytrace.h"
#endif

BatchOptions Batch::batchOptions;
const RayTracer *Batch::currentRayTracer = nullptr;

const BatchOptions *
Batch::batchGetOptions() {
    return &batchOptions;
}

void
Batch::generalParseOptions(int *argc, char **argv) {
    BatchOptionsParser::parse(argc, argv, batchOptions);
}

#ifdef RAYTRACING_ENABLED
void
Batch::batchRayTraceSaveImage(
    const char *fileName,
    java::OutputStream *outputStream,
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
        java::OutputStream *outputStream,
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
    java::OutputStream *outputStream = FileUncompressWrapper::openOutputStreamCompressWrapper(fileName, &isPipe);

    // Call the user supplied procedure to process the file
    processFileCallback(fileName, outputStream, isPipe, scene, radianceMethod, rayTracer, toneMapOptions, renderOptions);

    FileUncompressWrapper::closeOutputStream(outputStream);
}

void
Batch::batchSaveRadianceImage(
    const char *fileName,
    java::OutputStream *outputStream,
    const int isPipe,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RayTracer * /*rayTracer*/,
    ToneMappingContext *toneMapOptions,
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
        java::System::out.printf("Saving LOGLUV image to file '%s' ....... ", fileName);
    } else {
        java::System::out.printf("Saving RGB image to file '%s' .......... ", fileName);
    }
    java::System::out.flush();

    t = java::System::nanoTime();

    RadianceImageExporter::exportImage(fileName, outputStream, isPipe, scene, radianceMethod, toneMapOptions, renderOptions);

    java::System::out.printf(
        "%g secs.\n",
        static_cast<float>(static_cast<double>(java::System::nanoTime() - t) / 1000000000.0));
    Canvas::canvasPullMode();
}

void
Batch::batchSaveRadianceModel(
    const char *fileName,
    java::OutputStream *outputStream,
    const int /*isPipe*/,
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RayTracer */*rayTracer*/,
    ToneMappingContext * /*toneMapOptions*/,
    const RenderOptions *renderOptions)
{
    long long t;

    if ( outputStream == nullptr ) {
        return;
    }

    Canvas::canvasPushMode();
    java::System::out.printf("Saving VRML model to file '%s' ... ", fileName);
    java::System::out.flush();
    t = java::System::nanoTime();

    if ( radianceMethod != nullptr ) {
        radianceMethod->writeVRML(scene->camera, outputStream, renderOptions);
    }

    java::System::out.printf(
        "%g secs.\n",
        static_cast<float>(static_cast<double>(java::System::nanoTime() - t) / 1000000000.0));
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
    long long startTime;
    long long wasted_start;
    float wastedSecs;

    if ( scene->geometryList == nullptr || scene->geometryList->size() == 0 ) {
        java::System::out.printf("Empty world? Missing argument to some command line parameter option?\n");
        return;
    }

    Batch::currentRayTracer = rayTracer;

    startTime = java::System::nanoTime();
    wastedSecs = 0.0;

    if ( radianceMethod != nullptr ) {
        java::System::out.printf("Doing %s ...\n", radianceMethod->getRadianceMethodName());

        java::System::out.flush();
        java::System::err.flush();

        bool done = false;
        for ( int iterationNumber = 0;
              iterationNumber < batchOptions.iterations && !done;
              iterationNumber++ ) {
            java::System::out.printf("-----------------------------------\n"
               "radiance iteration %04d\n"
               "-----------------------------------\n\n", iterationNumber);

            Canvas::canvasPushMode();
            done = radianceMethod->doStep(scene, renderOptions);
            Canvas::canvasPullMode();

            java::System::out.flush();
            java::System::err.flush();

            java::System::out.printf("%s", radianceMethod->getStats());

            #ifdef OPEN_GL_ENABLED
                RenderOpenGL::renderGetNearFar(scene->camera, scene->geometryList);
            #endif

            java::System::out.flush();
            java::System::err.flush();

            wasted_start = java::System::nanoTime();

            if ( (!(iterationNumber % batchOptions.saveModulo)) && *batchOptions.radianceImageFileNameFormat ) {
                int n = static_cast<int>(strlen(batchOptions.radianceImageFileNameFormat)) + 1;
                char *fileName = new char[n];
                java::Formatter::format(
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
                int n = static_cast<int>(strlen(batchOptions.radianceModelFileNameFormat)) + 1;
                char *fileName = new char[n];
                java::Formatter::format(
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

            wastedSecs += static_cast<float>(
                static_cast<double>(wasted_start - java::System::nanoTime()) / 1000000000.0);

            java::System::out.flush();
            java::System::err.flush();
        }
    } else {
        java::System::out.printf("(No world-space radiance computations are being done)\n");
    }

    if ( batchOptions.timings ) {
        java::System::out.printf("Radiance total time %g secs.\n",
                static_cast<float>(static_cast<double>(java::System::nanoTime() - startTime) / 1000000000.0) - wastedSecs);
    }

    #ifdef RAYTRACING_ENABLED
        if ( Batch::currentRayTracer != nullptr ) {
            java::System::out.printf("Doing %s ...\n", Batch::currentRayTracer->getName());

            startTime = java::System::nanoTime();
            Raytrace::rayTraceExecute(
                nullptr,
                nullptr,
                false,
                scene,
                radianceMethod,
                Batch::currentRayTracer,
                toneMapOptions,
                renderOptions);

            if ( batchOptions.timings ) {
                java::System::out.printf("Raytracing total time %g secs.\n",
                        static_cast<float>(static_cast<double>(java::System::nanoTime() - startTime) / 1000000000.0));
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
            java::System::out.printf("(No pixel-based radiance computations are being done)\n");
        }
    #endif

    java::System::out.printf("Computations finished.\n");
    Batch::currentRayTracer = nullptr;
}
