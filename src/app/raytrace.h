#ifndef RPK_RAYTRACE_H
#define RPK_RAYTRACE_H

#include <cstdio>

#include "java/util/ArrayList.h"
#include "raycasting/common/Raytracer.h"

/**
If this is undefined, the raytracing code can be trimmed as follows:
- PHOTONMAP module can be removed
- All of the ray-casting module can be removed except the RayCaster class
*/
#define RAYTRACING_ENABLED true

extern void mainRayTracingDefaults();
extern void mainParseRayTracingOptions(int *argc, char **argv);
extern void mainSetRayTracingMethod(Raytracer *newMethod);
extern void batchSaveRaytracingImage(const char *fileName, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches);
extern void batchRayTrace(char *filename, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches);

#endif
