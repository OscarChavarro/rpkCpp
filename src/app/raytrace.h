#ifndef __RAYTRACE__
#define __RAYTRACE__

#include <cstdio>

#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "raycasting/common/Raytracer.h"

#ifdef RAYTRACING_ENABLED
    extern void mainRayTracingDefaults(java::ArrayList<Patch *> *lightSourcePatches);
    extern void mainParseRayTracingOptions(int *argc, char **argv);
    extern void mainSetRayTracingMethod(Raytracer *newMethod, java::ArrayList<Patch *> *lightSourcePatches);
    extern void batchSaveRaytracingImage(const char *fileName, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches, Geometry *clusteredWorldGeometry, RadianceMethod *context);

    extern void
    batchRayTrace(
        char *filename,
        FILE *fp,
        int isPipe,
        Background *sceneBackground,
        VoxelGrid *sceneWorldVoxelGrid,
        java::ArrayList<Patch *> *scenePatches,
        java::ArrayList<Patch *> *lightPatches,
        Geometry *clusteredWorldGeometry,
        RadianceMethod *context);

#endif

#endif
