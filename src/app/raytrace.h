#ifndef __RAYTRACE__
#define __RAYTRACE__

#include <cstdio>

#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "scene/Scene.h"
#include "raycasting/common/Raytracer.h"

#ifdef RAYTRACING_ENABLED
    extern void mainRayTracingDefaults();
    extern void rayTracingParseOptions(int *argc, char **argv);
    extern void mainSetRayTracingMethod(Raytracer *newMethod, java::ArrayList<Patch *> *lightSourcePatches);

    extern void
    batchSaveRaytracingImage(
        const char *fileName,
        FILE *fp,
        int isPipe,
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RenderOptions *renderOptions);

    extern void
    batchRayTrace(
        char *filename,
        FILE *fp,
        int isPipe,
        Scene *scene,
        RadianceMethod *radianceMethod,
        RenderOptions *renderOptions);

#endif

#endif
