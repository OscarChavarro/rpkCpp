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
static const int STRING_LENGTH = 1000;

static char globalRaytracingMethodsString[STRING_LENGTH];
static Raytracer *globalRayTracingMethods[] = {
    &GLOBAL_raytracing_stochasticMethod,
    &GLOBAL_raytracing_biDirectionalPathMethod,
    &GLOBAL_rayCasting_RayCasting,
    &GLOBAL_rayCasting_RayMatting,
    nullptr
};

static void
mainMakeRaytracingMethodsString() {
    char *str = globalRaytracingMethodsString;
    int n = 0;
    snprintf(str, 80, "-raytracing-method <method>: set pixel-based radiance computation method\n%n", &n);
    str = &str[n];
    snprintf(str, 80, "\tmethods: %-20.20s %s%s\n%n",
             "none", "no pixel-based radiance computation",
             !GLOBAL_raytracer_activeRaytracer ? " (default)" : "", &n);
    str += n;

    for ( Raytracer **window = globalRayTracingMethods; *window; window++ ) {
        const Raytracer *method = *window;
        snprintf(str, STRING_LENGTH, "\t         %-20.20s %s%s\n%n",
                 method->shortName, method->fullName,
                 GLOBAL_raytracer_activeRaytracer == method ? " (default)" : "", &n);
        str += n;
    }
    *(str - 1) = '\0'; // Discard last newline character
}

void
mainRayTracingDefaults() {
    for ( Raytracer **window = globalRayTracingMethods; *window; window++ ) {
        Raytracer *method = *window;
        method->Defaults();
        if ( strncasecmp(DEFAULT_RAYTRACING_METHOD, method->shortName, method->nameAbbrev) == 0 ) {
            mainSetRayTracingMethod(method, nullptr);
        }
    }
    mainMakeRaytracingMethodsString(); // Comes last
}

/**
This routine sets the current raytracing method to be used
*/
void
mainSetRayTracingMethod(Raytracer *newMethod, java::ArrayList<Patch *> *lightSourcePatches) {
    if ( GLOBAL_raytracer_activeRaytracer ) {
        GLOBAL_raytracer_activeRaytracer->Terminate();
    }

    GLOBAL_raytracer_activeRaytracer = newMethod;
    if ( GLOBAL_raytracer_activeRaytracer ) {
        GLOBAL_raytracer_activeRaytracer->Initialize(lightSourcePatches);
    }
}

void
batchSaveRaytracingImage(
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
    rayTracingParseOptions(argc, argv, globalRayTracingMethods, globalRaytracingMethodsString);
}

void
batchRayTrace(
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
