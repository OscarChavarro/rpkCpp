#ifndef __RAYTRACE__
#define __RAYTRACE__

#include "java/io/OutputStream.h"
#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "scene/Scene.h"
#include "raycasting/common/RayTracer.h"

#ifdef RAYTRACING_ENABLED
    extern RayTracer * rayTraceCreate(const Scene *scene, const char *rayTracerName);

    extern void
    rayTraceSaveImage(
        const char *fileName,
        java::io::OutputStream *stream,
        int isPipe,
        const Scene *scene,
        const RadianceMethod *,
        const RayTracer *rayTracer,
        const RenderOptions *);

    extern void
    rayTraceExecute(
        const char *filename,
        java::io::OutputStream *stream,
        int isPipe,
        Scene *scene,
        RadianceMethod *radianceMethod,
        const RayTracer *rayTracer,
        RenderOptions *renderOptions);

    extern void rayTraceParseOptions(int *argc, char **argv, char *rayTracerName);
#endif

#endif
