#include <cstdio>
#include <cstring>
#include <ctime>

#include "common/options.h"
#include "common/error.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracer.h"
#include "raycasting/raytracing/BidirectionalPathRaytracer.h"
#include "raycasting/simple/RayCaster.h"
#include "raycasting/simple/RayMatter.h"
#include "app/raytrace.h"
#include "material/statistics.h"
#include "common/RenderOptions.h"
#include "render/canvas.h"

// Raytracing defaults
#define DEFAULT_RAYTRACING_METHOD "stochastic"
#define STRING_SIZE 1000

extern java::ArrayList<Patch *> *GLOBAL_app_lightSourcePatches;

static char globalRaytracingMethodsString[STRING_SIZE];
static Raytracer *globalRayTracingMethods[] = {
    &GLOBAL_raytracing_stochasticMethod,
    &GLOBAL_raytracing_biDirectionalPathMethod,
    &GLOBAL_rayCasting_RayCasting,
    &GLOBAL_rayCasting_RayMatting,
    nullptr
};

static void
mainRayTracingOption(void *value) {
    char *name = *(char **) value;

    for ( Raytracer **methodpp = globalRayTracingMethods; *methodpp; methodpp++ ) {
        Raytracer *method = *methodpp;
        if ( strncasecmp(name, method->shortName, method->nameAbbrev) == 0 ) {
            mainSetRayTracingMethod(method);
            return;
        }
    }

    if ( strncasecmp(name, "none", 4) == 0 ) {
        mainSetRayTracingMethod(nullptr);
    } else {
        logError(nullptr, "Invalid raytracing method name '%s'", name);
    }
}

static CommandLineOptionDescription globalRaytracingOptions[] = {
        {"-raytracing-method", 4, Tstring,  nullptr, mainRayTracingOption, globalRaytracingMethodsString},
        {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

static void
mainMakeRaytracingMethodsString() {
    char *str = globalRaytracingMethodsString;
    int n;
    snprintf(str, 80, "-raytracing-method <method>: set pixel-based radiance computation method\n%n", &n);
    str += n;
    snprintf(str, 80, "\tmethods: %-20.20s %s%s\n%n",
             "none", "no pixel-based radiance computation",
             !GLOBAL_raytracer_activeRaytracer ? " (default)" : "", &n);
    str += n;

    Raytracer **methodpp;
    for ( methodpp = globalRayTracingMethods; *methodpp; methodpp++ ) {
        Raytracer *method = *methodpp;
        snprintf(str, STRING_SIZE, "\t         %-20.20s %s%s\n%n",
                 method->shortName, method->fullName,
                 GLOBAL_raytracer_activeRaytracer == method ? " (default)" : "", &n);
        str += n;
    }
    *(str - 1) = '\0'; // Discard last newline character
}

void
mainRayTracingDefaults() {
    for ( Raytracer **methodpp = globalRayTracingMethods; *methodpp; methodpp++ ) {
        Raytracer *method = *methodpp;
        method->Defaults();
        if ( strncasecmp(DEFAULT_RAYTRACING_METHOD, method->shortName, method->nameAbbrev) == 0 ) {
            mainSetRayTracingMethod(method);
        }
    }
    mainMakeRaytracingMethodsString(); // Comes last
}

void
mainParseRayTracingOptions(int *argc, char **argv) {
    parseOptions(globalRaytracingOptions, argc, argv);
    for ( Raytracer **methodpp = globalRayTracingMethods; *methodpp; methodpp++ ) {
        Raytracer *method = *methodpp;
        method->ParseOptions(argc, argv);
    }
}

/**
This routine sets the current raytracing method to be used
*/
void
mainSetRayTracingMethod(Raytracer *newMethod) {
    if ( GLOBAL_raytracer_activeRaytracer ) {
        GLOBAL_raytracer_activeRaytracer->InterruptRayTracing();
        GLOBAL_raytracer_activeRaytracer->Terminate();
    }

    GLOBAL_raytracer_activeRaytracer = newMethod;
    if ( GLOBAL_raytracer_activeRaytracer ) {
        GLOBAL_raytracer_activeRaytracer->Initialize(GLOBAL_app_lightSourcePatches);
    }
}

void
batchSaveRaytracingImage(const char *fileName, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches) {
    ImageOutputHandle *img = nullptr;
    clock_t t;

    if ( !fp ) {
        return;
    }

    t = clock();

    if ( fp ) {
        img = createRadianceImageOutputHandle(
                (char *) fileName,
                fp,
                isPipe,
                GLOBAL_camera_mainCamera.xSize,
                GLOBAL_camera_mainCamera.ySize,
                (float)GLOBAL_statistics_referenceLuminance / 179.0f);
        if ( !img ) {
            return;
        }
    }

    if ( !GLOBAL_raytracer_activeRaytracer ) {
        logWarning(nullptr, "No ray tracing method active");
    } else if ( !GLOBAL_raytracer_activeRaytracer->SaveImage || !GLOBAL_raytracer_activeRaytracer->SaveImage(img)) {
            logWarning(nullptr, "No previous %s image available", GLOBAL_raytracer_activeRaytracer->fullName);
        }

    if ( img != nullptr ) {
        deleteImageOutputHandle(img);
    }

    fprintf(stdout, "Raytrace save image: %g secs.\n", (float) (clock() - t) / (float) CLOCKS_PER_SEC);
}

void
batchRayTrace(char *filename, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches) {
    GLOBAL_render_renderOptions.renderRayTracedImage = true;
    GLOBAL_camera_mainCamera.changed = false;

    canvasPushMode();
    rayTrace(filename, fp, isPipe, GLOBAL_raytracer_activeRaytracer, scenePatches, lightPatches);
    canvasPullMode();
}