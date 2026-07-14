/**
Random walk generation
*/

#ifndef TRACE_PATH__
#define TRACE_PATH__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRaytracingPathNode.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Path.h"

class Tracepath final {
  public:
    using PatchProbabilityCallback = double (*)(const Patch *patch);
    using ScorePathCallback = void (*)(const Path *, long numberOfPaths, PatchProbabilityCallback birthProb);
    using UpdatePatchCallback = void (*)(const Patch *patch, double w);

  private:
    static PatchProbabilityCallback birthProbability;
    static double sumProbabilities;

    static void initPath(Path *path);
    static void clearPath(Path *path);
    static void pathAddNode(Path *path, Patch *patch, double probability, Vector3D inPoint, Vector3D outpoint);
    static void freePathNodes(Path *path);
    static Path *tracePath(
        const VoxelGrid *sceneWorldVoxelGrid,
        Patch *origin,
        double birthProb,
        PatchProbabilityCallback survivalProbabilityCallBack,
        Path *path);
    static double patchNormalisedBirthProbability(const Patch *patch);

  public:
    static void tracePaths(
        const VoxelGrid *sceneWorldVoxelGrid,
        long numberOfPaths,
        PatchProbabilityCallback birthProbabilityCallBack,
        PatchProbabilityCallback survivalProbabilityCallBack,
        ScorePathCallback scorePathCallBack,
        UpdatePatchCallback updateCallBack,
        const java::ArrayList<Patch *> *scenePatches);
};

#endif
