#ifndef RAYTRACE__
#define RAYTRACE__

#include "java/io/OutputStream.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/raycasting/common/RayTracer.h"
#include "vsdk/toolkit/raycasting/simple/RayMatterState.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/LightList.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

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
