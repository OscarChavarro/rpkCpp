#include <cstdio>
#include <cstring>
#include <ctime>

#include "common/error.h"
#include "common/Statistics.h"
#include "common/RenderOptions.h"
#include "render/canvas.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracer.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathRaytracer.h"
#include "raycasting/simple/RayCaster.h"
#include "raycasting/simple/RayMatter.h"
#include "app/raytrace.h"
#include "app/commandLine.h"

#ifdef RAYTRACING_ENABLED

// Raytracing defaults
static const char* DEFAULT_RAYTRACING_METHOD = "stochastic";
static char globalRayTracerName[1000];

static Raytracer *globalRayTracingMethods[] = {
    &GLOBAL_raytracing_stochasticMethod,
    &GLOBAL_raytracing_biDirectionalPathMethod,
    &GLOBAL_rayCasting_RayCasting,
    &GLOBAL_rayCasting_RayMatting,
    nullptr
};

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

static void
rayTracePrepareRayTracer(const char *rayTracerName, const Scene *scene) {
    for ( Raytracer **window = globalRayTracingMethods; *window; window++ ) {
        Raytracer *method = *window;
        if ( strncasecmp(rayTracerName, method->shortName, method->nameAbbrev) == 0 ) {
            rayTraceSetMethod(method, scene->lightSourcePatchList);
            return;
        }
    }

    if ( strncasecmp(rayTracerName, "none", 4) == 0 ) {
        rayTraceSetMethod(nullptr, scene->lightSourcePatchList);
    } else {
        logError(nullptr, "Invalid raytracing method name '%s'", rayTracerName);
    }
}

void
rayTraceDefaults(const Scene *scene) {
    rayTracePrepareRayTracer(globalRayTracerName, scene);

    Raytracer *method = GLOBAL_raytracer_activeRaytracer;
    if ( method != nullptr ) {
        if ( strncasecmp(DEFAULT_RAYTRACING_METHOD, method->shortName, method->nameAbbrev) == 0 ) {
            rayTraceSetMethod(method, nullptr);
        }
        if ( method->Defaults != nullptr ) {
            method->Defaults();
        }
    }
}

/**
This routine sets the current raytracing method to be used
*/
void
rayTraceSetMethod(Raytracer *newMethod, java::ArrayList<Patch *> *lightSourcePatches) {
    if ( GLOBAL_raytracer_activeRaytracer ) {
        GLOBAL_raytracer_activeRaytracer->Terminate();
    }

    GLOBAL_raytracer_activeRaytracer = newMethod;
    if ( GLOBAL_raytracer_activeRaytracer ) {
        GLOBAL_raytracer_activeRaytracer->Initialize(lightSourcePatches);
    }
}

void
rayTraceSaveImage(
    const char *fileName,
    FILE *fp,
    int isPipe,
    const Scene *scene,
    const RadianceMethod * /*radianceMethod*/,
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
        scene->camera->ySize,
        (float)GLOBAL_statistics.referenceLuminance / 179.0f);
    if ( !img ) {
        return;
    }

    if ( !GLOBAL_raytracer_activeRaytracer ) {
        logWarning(nullptr, "No ray tracing method active");
    } else if ( !GLOBAL_raytracer_activeRaytracer->SaveImage || !GLOBAL_raytracer_activeRaytracer->SaveImage(img) ) {
        logWarning(nullptr, "No previous %s image available", GLOBAL_raytracer_activeRaytracer->fullName);
    }

    deleteImageOutputHandle(img);

    fprintf(stdout, "Raytrace save image: %g secs.\n", (float) (clock() - t) / (float) CLOCKS_PER_SEC);
}

void
rayTraceParseOptions(int *argc, char **argv) {
    char helpMessage[1000];

    rayTraceMakeMethodsHelpMessage(helpMessage);
    rayTracingParseOptions(argc, argv, helpMessage, globalRayTracerName);
}

void
rayTraceExecute(
    const char *filename,
    FILE *fp,
    int isPipe,
    Scene *scene,
    RadianceMethod *radianceMethod,
    RenderOptions *renderOptions)
{
    renderOptions->renderRayTracedImage = true;
    scene->camera->changed = false;

    canvasPushMode();
    rayTrace(
        filename,
        fp,
        isPipe,
        GLOBAL_raytracer_activeRaytracer,
        scene,
        radianceMethod,
        renderOptions);
    canvasPullMode();
}

#endif
