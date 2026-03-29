/**
Random walk generation
*/

#ifndef __TRACE_PATH__
#define __TRACE_PATH__

#include "java/util/ArrayList.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracingPathNode.h"
#include "raycasting/stochasticRaytracing/Path.h"

extern void
tracePaths(
    const VoxelGrid *sceneWorldVoxelGrid,
    long numberOfPaths,
    double (*birthProbabilityCallBack)(const Patch *P),
    double (*survivalProbabilityCallBack)(const Patch *P),
    void (*scorePathCallBack)(const Path *, long nr_paths, double (*birth_prob)(const Patch *)),
    void (*updateCallBack)(const Patch *P, double w),
    const java::ArrayList<Patch *> *scenePatches);

#endif
