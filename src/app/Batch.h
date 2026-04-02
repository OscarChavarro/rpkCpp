#ifndef __BATCH__
#define __BATCH__

#include "java/io/OutputStream.h"
#include "java/util/ArrayList.h"
#include "raycasting/common/RayTracer.h"
#include "scene/RadianceMethod.h"
#include "skin/Patch.h"
#include "scene/Scene.h"
#include "app/options/BatchOptions.h"
#include "app/options/OptionsType.h"

class Batch final {
  public:
    static void batchExecuteRadianceSimulation(
        Scene *scene,
        RadianceMethod *radianceMethod,
        const RayTracer *rayTracer,
        RenderOptions *renderOptions);
    static void generalParseOptions(int *argc, char **argv, OptionsType &optionTypes);
    static const BatchOptions *batchGetOptions();

  private:
    static BatchOptions batchOptions;
    static const RayTracer *currentRayTracer;

#ifdef RAYTRACING_ENABLED
    static void batchRayTraceSaveImage(
        const char *fileName,
        java::OutputStream *outputStream,
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
            java::OutputStream *outputStream,
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
        java::OutputStream *outputStream,
        int isPipe,
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RayTracer *rayTracer,
        const RenderOptions *renderOptions);
    static void batchSaveRadianceModel(
        const char *fileName,
        java::OutputStream *outputStream,
        int isPipe,
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RayTracer *rayTracer,
        const RenderOptions *renderOptions);
};

#endif
