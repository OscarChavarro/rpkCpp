#ifndef __RAYTRACE__
#define __RAYTRACE__

#include <cstdio>

#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "scene/Scene.h"
#include "raycasting/common/Raytracer.h"

#ifdef RAYTRACING_ENABLED
    extern RayTracer * rayTraceCreate(const Scene *scene, const char *rayTracerName);

    extern void
    rayTraceSaveImage(
        const char *fileName,
        FILE *fp,
        int isPipe,
        const Scene *scene,
        const RadianceMethod *,
        const RenderOptions *);

    extern void
    rayTraceExecute(
        const char *filename,
        FILE *fp,
        int isPipe,
        Scene *scene,
        RadianceMethod *radianceMethod,
        RenderOptions *renderOptions);

    extern void rayTraceParseOptions(int *argc, char **argv, char *rayTracerName);
#endif

#endif
