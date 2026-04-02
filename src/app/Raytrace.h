#ifndef __RAYTRACE__
#define __RAYTRACE__

#include "java/io/OutputStream.h"
#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "scene/Scene.h"
#include "raycasting/common/RayTracer.h"

#ifdef RAYTRACING_ENABLED
class RayMatterState;
class BidirectionalPathTracingState;
class StochasticRayTracingState;
class LightList;
class OptionsType;

class Raytrace final {
  public:
    static RayTracer *rayTraceCreate(
        const Scene *scene,
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
        RenderOptions *renderOptions);
    static void rayTraceParseOptions(
            int *argc,
            char **argv,
            char *rayTracerName,
            OptionsType &optionTypes);

  private:
    static void
    rayTraceSetMethod(
        const RayTracer *rayTracer,
        const java::ArrayList<Patch *> *lightSourcePatches,
        LightList *&lightList);
    static RayTracer *rayTraceCreateRayTracerFromName(
        const char *rayTracerName,
        const Scene *scene,
        RayMatterState &rayMatterState,
        BidirectionalPathTracingState &bidirectionalPathState,
        StochasticRayTracingState &stochasticRayTracingState,
        LightList *&lightList);
};
#endif

#endif
