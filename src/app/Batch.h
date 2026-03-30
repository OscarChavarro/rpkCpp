#ifndef __BATCH__
#define __BATCH__

#include "java/io/OutputStream.h"
#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "scene/Scene.h"
#include "app/BatchOptions.h"

class RayTracer;
class RadianceMethod;

class Batch final {
  public:
    static void batchExecuteRadianceSimulation(
        Scene *scene,
        RadianceMethod *radianceMethod,
        const RayTracer *rayTracer,
        RenderOptions *renderOptions);
    static void generalParseOptions(int *argc, char **argv);
    static const BatchOptions *batchGetOptions();

  private:
    static void openGlSaveScreen(
        const char *fileName,
        java::io::OutputStream *outputStream,
        int isPipe,
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RenderOptions *renderOptions);
#ifdef RAYTRACING_ENABLED
    static void batchRayTraceSaveImage(
        const char *fileName,
        java::io::OutputStream *outputStream,
        int isPipe,
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RayTracer *rayTracer,
        const RenderOptions *renderOptions);
#endif
    static void batchProcessFile(
        const char *fileName,
        void (*processFileCallback)(
            const char *fileName,
            java::io::OutputStream *outputStream,
            int isPipe,
            const Scene *scene,
            const RadianceMethod *radianceMethod,
            const RayTracer *rayTracer,
            const RenderOptions *renderOptions),
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RayTracer *rayTracer,
        const RenderOptions *renderOptions);
    static void batchSaveRadianceImage(
        const char *fileName,
        java::io::OutputStream *outputStream,
        int isPipe,
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RayTracer *rayTracer,
        const RenderOptions *renderOptions);
    static void batchSaveRadianceModel(
        const char *fileName,
        java::io::OutputStream *outputStream,
        int isPipe,
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RayTracer *rayTracer,
        const RenderOptions *renderOptions);
};

#endif
