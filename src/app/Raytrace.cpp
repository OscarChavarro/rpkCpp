#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "java/lang/System.h"
#include <cstring>

#include "common/Error.h"
#include "render/Canvas.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracer.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathRaytracer.h"
#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "raycasting/simple/RayCaster.h"
#include "raycasting/simple/RayMatter.h"
#include "app/Raytrace.h"
#include "app/options/RaytraceOptionsParser.h"

/**
This routine sets the current raytracing method to be used
*/
void
Raytrace::rayTraceSetMethod(
    const RayTracer *rayTracer,
    const java::ArrayList<Patch *> *lightSourcePatches,
    LightList *&lightList)
{
    (void) lightList;
    if ( rayTracer != nullptr ) {
        rayTracer->initialize(lightSourcePatches);
    }
}

RayTracer *
Raytrace::rayTraceCreateRayTracerFromName(
    const char *rayTracerName,
    const Scene *scene,
    RayMatterState &rayMatterState,
    BidirectionalPathTracingState &bidirectionalPathState,
    StochasticRayTracingState &stochasticRayTracingState,
    LightList *&lightList)
{
    RayTracer *newRaytracer;
    if ( strcmp(rayTracerName, "RayMatting") == 0 ) {
        newRaytracer = new RayMatter(nullptr, scene->camera, rayMatterState, scene->toneMapOptions);
    } else if ( strcmp(rayTracerName, "RayCasting") == 0 ) {
        newRaytracer = new RayCaster(nullptr, scene->camera, scene->toneMapOptions);
    } else if ( strcmp(rayTracerName, "BidirectionalPathTracing") == 0 ) {
        newRaytracer = new BidirectionalPathRaytracer(bidirectionalPathState, lightList);
    } else if ( strcmp(rayTracerName, "StochasticRaytracing") == 0 ) {
        newRaytracer = new StochasticRaytracer(lightList, stochasticRayTracingState);
    } else {
        newRaytracer = nullptr;
    }
    Raytrace::rayTraceSetMethod(newRaytracer, scene->lightSourcePatchList, lightList);

    if ( newRaytracer == nullptr && strncasecmp(rayTracerName, "none", 4) != 0 ) {
        Error::error(nullptr, "Invalid raytracing method name '%s'", rayTracerName);
    }

    return newRaytracer;
}

RayTracer *
Raytrace::rayTraceCreate(
    const Scene *scene,
    const char *rayTracerName,
    RayMatterState &rayMatterState,
    BidirectionalPathTracingState &bidirectionalPathState,
    StochasticRayTracingState &stochasticRayTracingState,
    LightList *&lightList)
{
    RayTracer *rayTracer = Raytrace::rayTraceCreateRayTracerFromName(
        rayTracerName,
        scene,
        rayMatterState,
        bidirectionalPathState,
        stochasticRayTracingState,
        lightList);

    if ( rayTracer != nullptr ) {
        rayTracer->defaults();
    }
    return rayTracer;
}

void
Raytrace::rayTraceSaveImage(
    const char *fileName,
    java::OutputStream *stream,
    int isPipe,
    const Scene *scene,
    const RayTracer *rayTracer)
{
    long long t;

    if ( stream == nullptr ) {
        return;
    }

    t = java::System::nanoTime();

    ImageOutputHandle *img = ImageOutputHandle::createRadianceImageOutputHandle(
        fileName,
        stream,
        isPipe,
        scene->camera->xSize,
        scene->camera->ySize);
    if ( !img ) {
        return;
    }

    if ( rayTracer == nullptr ) {
        Error::warning(nullptr, "No ray tracing method active");
    } else if ( !rayTracer->saveImage(img) ) {
        Error::warning(nullptr, "No previous %s image available", rayTracer->getName());
    }

    ImageOutputHandle::deleteImageOutputHandle(img);

    java::System::out.printf(
        "Raytrace save image: %g secs.\n",
        static_cast<float>(static_cast<double>(java::System::nanoTime() - t) / 1000000000.0));
}

void
Raytrace::rayTraceParseOptions(
        int *argc,
        char **argv,
        char *rayTracerName,
        OptionsType &optionTypes)
{
    RaytraceOptionsParser::parse(argc, argv, rayTracerName, optionTypes);
}

void
Raytrace::rayTraceExecute(
    const char *filename,
    java::OutputStream *stream,
    int isPipe,
    Scene *scene,
    RadianceMethod *radianceMethod,
    const RayTracer *rayTracer,
    RenderOptions *renderOptions)
{
    renderOptions->renderRayTracedImage = true;
    scene->camera->changed = false;

    Canvas::canvasPushMode();
    RayTracer::rayTrace(
        filename,
        stream,
        isPipe,
        rayTracer,
        scene,
        radianceMethod,
        renderOptions);
    Canvas::canvasPullMode();
}

#endif
