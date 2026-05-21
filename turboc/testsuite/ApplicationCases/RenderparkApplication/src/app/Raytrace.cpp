#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "java/lang/System.h"
#include <string.h>

#include "common/logging/Logger.h"
#include "render/Canvas.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracer.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathRaytracer.h"
#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "raycasting/simple/RayCaster.h"
#include "raycasting/simple/RayMatter.h"
#include "app/Raytrace.h"
#include "app/options/OptionsGroupRaytracing.h"

/**
This routine sets the current raytracing method to be used
*/
void
Raytrace::rayTraceSetMethod(
    const RayTracer *rayTracer,
    const ArrayList<Patch *> *lightSourcePatches,
    LightList *&lightList)
{
    (void) lightList;
    if ( rayTracer != NULL ) {
        rayTracer->initialize(lightSourcePatches);
    }
}

RayTracer *
Raytrace::rayTraceCreateRayTracerFromName(
    const char *rayTracerName,
    const Scene *scene,
    ToneMappingContext *toneMapOptions,
    RayMatterState &rayMatterState,
    BidirectionalPathTracingState &bidirectionalPathState,
    StochasticRayTracingState &stochasticRayTracingState,
    LightList *&lightList)
{
    RayTracer *newRaytracer;
    if ( strcmp(rayTracerName, "RayMatting") == 0 ) {
        newRaytracer = new RayMatter(NULL, scene->camera, rayMatterState, toneMapOptions);
    } else if ( strcmp(rayTracerName, "RayCasting") == 0 ) {
        newRaytracer = new RayCaster(NULL, scene->camera, toneMapOptions);
    } else if ( strcmp(rayTracerName, "BidirectionalPathTracing") == 0 ) {
        newRaytracer = new BidirectionalPathRaytracer(bidirectionalPathState, lightList);
    } else if ( strcmp(rayTracerName, "StochasticRaytracing") == 0 ) {
        newRaytracer = new StochasticRaytracer(lightList, stochasticRayTracingState);
    } else {
        newRaytracer = NULL;
    }
    Raytrace::rayTraceSetMethod(newRaytracer, scene->lightSourcePatchList, lightList);

    if ( newRaytracer == NULL && strncasecmp(rayTracerName, "none", 4) != 0 ) {
        Logger::error(NULL, "Invalid raytracing method name '%s'", rayTracerName);
    }

    return newRaytracer;
}

RayTracer *
Raytrace::rayTraceCreate(
    const Scene *scene,
    ToneMappingContext *toneMapOptions,
    const char *rayTracerName,
    RayMatterState &rayMatterState,
    BidirectionalPathTracingState &bidirectionalPathState,
    StochasticRayTracingState &stochasticRayTracingState,
    LightList *&lightList)
{
    RayTracer *rayTracer = Raytrace::rayTraceCreateRayTracerFromName(
        rayTracerName,
        scene,
        toneMapOptions,
        rayMatterState,
        bidirectionalPathState,
        stochasticRayTracingState,
        lightList);

    if ( rayTracer != NULL ) {
        rayTracer->defaults();
    }
    return rayTracer;
}

void
Raytrace::rayTraceSaveImage(
    const char *fileName,
    OutputStream *stream,
    int isPipe,
    const Scene *scene,
    const RayTracer *rayTracer)
{
    long t;

    if ( stream == NULL ) {
        return;
    }

    t = System::nanoTime();

    ImageOutputHandle *img = ImageOutputHandle::createRadianceImageOutputHandle(
        fileName,
        stream,
        isPipe,
        scene->camera->xSize,
        scene->camera->ySize);
    if ( !img ) {
        return;
    }

    if ( rayTracer == NULL ) {
        Logger::warning(NULL, "No ray tracing method active");
    } else if ( !rayTracer->saveImage(img) ) {
        Logger::warning(NULL, "No previous %s image available", rayTracer->getName());
    }

    ImageOutputHandle::deleteImageOutputHandle(img);

    System::out.printf(
        "Raytrace save image: %g secs.\n",
        ((float)(((double)(System::nanoTime() - t)) / 1000000000.0)));
}

void
Raytrace::rayTraceParseOptions(
        int *argc,
        char **argv,
        char *rayTracerName)
{
    OptionsGroupRaytracing::parse(argc, argv, rayTracerName);
}

void
Raytrace::rayTraceExecute(
    const char *filename,
    OutputStream *stream,
    int isPipe,
    Scene *scene,
    RadianceMethod *radianceMethod,
    const RayTracer *rayTracer,
    ToneMappingContext *toneMapOptions,
    RenderOptions *renderOptions)
{
    renderOptions->setRenderRayTracedImage(true);
    scene->camera->changed = false;

    Canvas::canvasPushMode();
    RayTracer::rayTrace(
        filename,
        stream,
        isPipe,
        rayTracer,
        scene,
        radianceMethod,
        toneMapOptions,
        renderOptions);
    Canvas::canvasPullMode();
}

#endif
