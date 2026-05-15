#ifndef __BATCH__
#define __BATCH__

#include "java/io/OutputStream.h"
#include "java/util/ArrayList.h"
#include "raycasting/common/RayTracer.h"
#include "scene/RadianceMethod.h"
#include "environment/geometry/elements/Patch.h"
#include "scene/Scene.h"
#include "app/options/BatchOptions.h"
#include "tonemap/ToneMappingContext.h"

class Batch{ public:
    static void batchExecuteRadianceSimulation( Scene *scene, RadianceMethod *radianceMethod, const RayTracer *rayTracer, ToneMappingContext *toneMapOptions, RenderOptions *renderOptions);
    static void generalParseOptions(int *argc, char **argv);
    static const BatchOptions *batchGetOptions();

  private:
    static BatchOptions batchOptions;
    static const RayTracer *currentRayTracer;

#ifdef RAYTRACING_ENABLED
    static void batchRayTraceSaveImage( const char *fileName, OutputStream *outputStream, int isPipe, const Scene *scene, const RadianceMethod *radianceMethod, const RayTracer *rayTracer, ToneMappingContext *toneMapOptions, const RenderOptions *renderOptions);
#endif
    static void batchProcessFile( const char *fileName, void (*processFileCallback)( const char *fileName, OutputStream *outputStream, int isPipe, const Scene *scene, const RadianceMethod *radianceMethod, const RayTracer *rayTracer, ToneMappingContext *toneMapOptions, const RenderOptions *renderOptions), const Scene *scene, const RadianceMethod *radianceMethod, const RayTracer *rayTracer, ToneMappingContext *toneMapOptions, const RenderOptions *renderOptions);
    static void batchSaveRadianceImage( const char *fileName, OutputStream *outputStream, int isPipe, const Scene *scene, const RadianceMethod *radianceMethod, const RayTracer *rayTracer, ToneMappingContext *toneMapOptions, const RenderOptions *renderOptions);
    static void batchSaveRadianceModel( const char *fileName, OutputStream *outputStream, int isPipe, const Scene *scene, const RadianceMethod *radianceMethod, const RayTracer *rayTracer, ToneMappingContext *toneMapOptions, const RenderOptions *renderOptions);
};

#endif
