#ifndef __RAYTRACE__
#define __RAYTRACE__

#include "java/io/OutputStream.h"
#include "java/util/ArrayList.h"
#include "material/RendererConfiguration.h"
#include "scene/Scene.h"
#include "raycasting/common/RayTracer.h"
#include "raycasting/simple/RayMatterState.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "tonemap/ToneMappingContext.h"

#ifdef RAYTRACING_ENABLED
class Raytrace final {
  public:
    static RayTracer *rayTraceCreate(
        const Scene *scene,
        ToneMappingContext *toneMapOptions,
        const char *rayTracerName,
        RayMatterState &rayMatterState,
        BidirectionalPathTracingState &bidirectionalPathState,
        StochasticRayTracingState &stochasticRayTracingState,
        LightList *&lightList);
    static void rayTraceSaveImage(
        const char *fileName,
        java::OutputStream *stream,
        int isPipe,
        const Scene *scene,
        const RayTracer *rayTracer);
    static void rayTraceExecute(
        const char *filename,
        java::OutputStream *stream,
        int isPipe,
        Scene *scene,
        RadianceMethod *radianceMethod,
        const RayTracer *rayTracer,
        ToneMappingContext *toneMapOptions,
        RendererConfiguration *renderOptions);
    static void rayTraceParseOptions(
            int *argc,
            char **argv,
            char *rayTracerName);

  private:
    static void
    rayTraceSetMethod(
        const RayTracer *rayTracer,
        const java::ArrayList<Patch *> *lightSourcePatches,
        LightList *&lightList);
    static RayTracer *rayTraceCreateRayTracerFromName(
        const char *rayTracerName,
        const Scene *scene,
        ToneMappingContext *toneMapOptions,
        RayMatterState &rayMatterState,
        BidirectionalPathTracingState &bidirectionalPathState,
        StochasticRayTracingState &stochasticRayTracingState,
        LightList *&lightList);
};
#endif

#endif
