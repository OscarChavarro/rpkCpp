#include "common/RenderOptions.h"
#ifdef RAYTRACING_ENABLED

#include <cstdio>
#include <cstring>
#include <ctime>

#include "common/error.h"
#include "common/Statistics.h"
#include "render/canvas.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracer.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathRaytracer.h"
#include "raycasting/simple/RayCaster.h"
#include "raycasting/simple/RayMatter.h"
#include "app/raytrace.h"
#include "app/commandLine.h"

static void
rayTraceMakeMethodsHelpMessage(char *str) {
    snprintf(str, 1000,
         "-raytracing-method <method>: set pixel-based radiance computation method\n"
         "\tmethods: none                 no pixel-based radiance computation\n"
         "\t         StochasticRaytracing Stochastic Raytracing & Final Gathers (default)\n"
         "\t         BidirectionalPathTra Bidirectional Path Tracing\n"
         "\t         RayCasting           Ray Casting\n"
         "\t         RayMatting           Ray Matting");
}

/**
This routine sets the current raytracing method to be used
*/
static void
rayTraceSetMethod(const RayTracer *rayTracer, const java::ArrayList<Patch *> *lightSourcePatches) {
    if ( rayTracer != nullptr ) {
        rayTracer->initialize(lightSourcePatches);
    }
}

static RayTracer *
rayTraceCreateRayTracerFromName(const char *rayTracerName, const Scene *scene) {
    RayTracer *newRaytracer;
    if ( strcmp(rayTracerName, "RayMatting") == 0 ) {
        newRaytracer = new RayMatter(nullptr, scene->camera);
    } else if ( strcmp(rayTracerName, "RayCasting") == 0 ) {
        newRaytracer = new RayCaster(nullptr, scene->camera);
    } else if ( strcmp(rayTracerName, "BidirectionalPathTracing") == 0 ) {
        newRaytracer = new BidirectionalPathRaytracer();
    } else if ( strcmp(rayTracerName, "StochasticRaytracing") == 0 ) {
        newRaytracer = new StochasticRaytracer();
    } else {
        newRaytracer = nullptr;
    }
    rayTraceSetMethod(newRaytracer, scene->lightSourcePatchList);

    if ( newRaytracer == nullptr && strncasecmp(rayTracerName, "none", 4) != 0 ) {
        logError(nullptr, "Invalid raytracing method name '%s'", rayTracerName);
    }

    return newRaytracer;
}

RayTracer *
rayTraceCreate(const Scene *scene, const char *rayTracerName) {
    RayTracer *rayTracer = rayTraceCreateRayTracerFromName(rayTracerName, scene);

    if ( rayTracer != nullptr ) {
        rayTracer->defaults();
    }
    return rayTracer;
}

void
rayTraceSaveImage(
    const char *fileName,
    FILE *fp,
    int isPipe,
    const Scene *scene,
    const RadianceMethod * /*radianceMethod*/,
    const RayTracer *rayTracer,
    const RenderOptions * /*renderOptions*/)
{
    clock_t t;

    if ( fp == nullptr ) {
        return;
    }

    t = clock();

    ImageOutputHandle *img = createRadianceImageOutputHandle(
        fileName,
        fp,
        isPipe,
        scene->camera->xSize,
        scene->camera->ySize);
    if ( !img ) {
        return;
    }

    if ( rayTracer == nullptr ) {
        logWarning(nullptr, "No ray tracing method active");
    } else if ( !rayTracer->saveImage(img) ) {
        logWarning(nullptr, "No previous %s image available", rayTracer->getName());
    }

    deleteImageOutputHandle(img);

    fprintf(stdout, "Raytrace save image: %g secs.\n", (float) (clock() - t) / (float) CLOCKS_PER_SEC);
}

void
rayTraceParseOptions(int *argc, char **argv, char *rayTracerName) {
    char helpMessage[1000];

    rayTraceMakeMethodsHelpMessage(helpMessage);
    strcpy(rayTracerName, "none");
    rayTracingParseOptions(argc, argv, helpMessage, rayTracerName);
}

void
rayTraceExecute(
    const char *filename,
    FILE *fp,
    int isPipe,
    Scene *scene,
    RadianceMethod *radianceMethod,
    const RayTracer *rayTracer,
    RenderOptions *renderOptions)
{
    renderOptions->renderRayTracedImage = true;
    scene->camera->changed = false;

    canvasPushMode();
    rayTrace(
        filename,
        fp,
        isPipe,
        rayTracer,
        scene,
        radianceMethod,
        renderOptions);
    canvasPullMode();
}

#endif
