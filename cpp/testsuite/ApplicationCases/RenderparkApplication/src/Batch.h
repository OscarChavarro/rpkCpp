#ifndef BATCH__
#define BATCH__

#include "java/io/OutputStream.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/raycasting/common/RayTracer.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "options/BatchOptions.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

class Batch final {
  public:
    static void batchExecuteRadianceSimulation(
        Scene *scene,
        RadianceMethod *radianceMethod,
        const RayTracer *rayTracer,
        ToneMappingContext *toneMapOptions,
        RendererConfiguration *renderOptions);
    static void generalParseOptions(int *argc, char **argv);
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
        ToneMappingContext *toneMapOptions,
        const RendererConfiguration *renderOptions);
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
            ToneMappingContext *toneMapOptions,
            const RendererConfiguration *renderOptions),
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RayTracer *rayTracer,
        ToneMappingContext *toneMapOptions,
        const RendererConfiguration *renderOptions);
    static void batchSaveRadianceImage(
        const char *fileName,
        java::OutputStream *outputStream,
        int isPipe,
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RayTracer *rayTracer,
        ToneMappingContext *toneMapOptions,
        const RendererConfiguration *renderOptions);
    static void batchSaveRadianceModel(
        const char *fileName,
        java::OutputStream *outputStream,
        int isPipe,
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RayTracer *rayTracer,
        ToneMappingContext *toneMapOptions,
        const RendererConfiguration *renderOptions);
};

#endif
