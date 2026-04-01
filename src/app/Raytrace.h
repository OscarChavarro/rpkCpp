#ifndef __RAYTRACE__
#define __RAYTRACE__

#include "java/io/OutputStream.h"
#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "scene/Scene.h"
#include "raycasting/common/RayTracer.h"

#ifdef RAYTRACING_ENABLED
class Raytrace final {
  public:
    static RayTracer *rayTraceCreate(const Scene *scene, const char *rayTracerName);
    static void rayTraceSaveImage(
        const char *fileName,
        java::OutputStream *stream,
        int isPipe,
        const Scene *scene,
        const RayTracer *rayTracer);
    static void rayTraceExecute(
        const char *filename,
        java::OutputStream *stream,
        int isPipe,
        Scene *scene,
        RadianceMethod *radianceMethod,
        const RayTracer *rayTracer,
        RenderOptions *renderOptions);
    static void rayTraceParseOptions(int *argc, char **argv, char *rayTracerName);

  private:
    static void rayTraceMakeMethodsHelpMessage(char *str);
    static void rayTraceSetMethod(const RayTracer *rayTracer, const java::ArrayList<Patch *> *lightSourcePatches);
    static RayTracer *rayTraceCreateRayTracerFromName(const char *rayTracerName, const Scene *scene);
};
#endif

#endif
