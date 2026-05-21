#ifndef __RAYTRACE__
#define __RAYTRACE__

#include "vsdk/java/io/OutputStream.h"
#include "vsdk/java/util/ArrayList.h"
#include "vsdk/material/RendererConfiguration.h"
#include "vsdk/scene/Scene.h"
#include "vsdk/raycasting/common/RayTracer.h"
#include "vsdk/raycasting/simple/RayMatterState.h"
#include "vsdk/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "vsdk/raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "vsdk/raycasting/bidirectionalRaytracing/LightList.h"
#include "vsdk/tonemap/ToneMappingContext.h"

#ifdef RAYTRACING_ENABLED
class Raytrace{ public:
    static RayTracer *rayTraceCreate( const Scene *scene, ToneMappingContext *toneMapOptions, const char *rayTracerName, RayMatterState &rayMatterState, BidirectionalPathTracingState &bidirectionalPathState, StochasticRayTracingState &stochasticRayTracingState, LightList *&lightList);
    static void rayTraceSaveImage( const char *fileName, OutputStream *stream, int isPipe, const Scene *scene, const RayTracer *rayTracer);
    static void rayTraceExecute( const char *filename, OutputStream *stream, int isPipe, Scene *scene, RadianceMethod *radianceMethod, const RayTracer *rayTracer, ToneMappingContext *toneMapOptions, RenderOptions *renderOptions);
    static void rayTraceParseOptions( int *argc, char **argv, char *rayTracerName);

  private:
    static void
    rayTraceSetMethod( const RayTracer *rayTracer, const ArrayList<Patch *> *lightSourcePatches, LightList *&lightList);
    static RayTracer *rayTraceCreateRayTracerFromName( const char *rayTracerName, const Scene *scene, ToneMappingContext *toneMapOptions, RayMatterState &rayMatterState, BidirectionalPathTracingState &bidirectionalPathState, StochasticRayTracingState &stochasticRayTracingState, LightList *&lightList);
};
#endif

#endif
