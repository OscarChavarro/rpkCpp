/**
Random walk generation
*/

#ifndef __TRACE_PATH__
#define __TRACE_PATH__

#include "java/util/ArrayList.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracingPathNode.h"
#include "raycasting/stochasticRaytracing/Path.h"

class Tracepath final {
  private:
    static double (*birthProbability)(const Patch *);
    static double sumProbabilities;

    static void initPath(Path *path);
    static void clearPath(Path *path);
    static void pathAddNode(Path *path, Patch *patch, double probability, Vector3D inPoint, Vector3D outpoint);
    static void freePathNodes(Path *path);
    static Path *tracePath(
        const VoxelGrid *sceneWorldVoxelGrid,
        Patch *origin,
        double birthProbability,
        double (*survivalProbabilityCallBack)(const Patch *patch),
        Path *path);
    static double patchNormalisedBirthProbability(const Patch *patch);

  public:
    static void tracePaths(
        const VoxelGrid *sceneWorldVoxelGrid,
        long numberOfPaths,
        double (*birthProbabilityCallBack)(const Patch *patch),
        double (*survivalProbabilityCallBack)(const Patch *patch),
        void (*scorePathCallBack)(const Path *, long numberOfPaths, double (*birthProb)(const Patch *)),
        void (*updateCallBack)(const Patch *patch, double w),
        const java::ArrayList<Patch *> *scenePatches);
};

#endif
