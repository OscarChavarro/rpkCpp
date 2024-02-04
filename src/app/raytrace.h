#ifndef RPK_RAYTRACE_H
#define RPK_RAYTRACE_H

#include <cstdio>

#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "raycasting/common/Raytracer.h"

#ifdef RAYTRACING_ENABLED
    extern void mainRayTracingDefaults();
    extern void mainParseRayTracingOptions(int *argc, char **argv);
    extern void mainSetRayTracingMethod(Raytracer *newMethod);
    extern void batchSaveRaytracingImage(const char *fileName, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches);
    extern void batchRayTrace(char *filename, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches);
#endif

#endif
