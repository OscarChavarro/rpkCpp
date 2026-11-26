/**
Random walk generation
*/

#ifndef __TRACEPATH__
#define __TRACEPATH__

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
    static void pathAddNode(Path *path, const Patch *patch, double probability, Vector3Dd inPoint, Vector3Dd outpoint);
    static void freePathNodes(Path *path);
    static Path *tracePath(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        Patch *origin,
        double birthProb,
        PatchProbabilityCallback survivalProbabilityCallBack,
        Path *path);
    static double patchNormalisedBirthProbability(const Patch *patch);

  public:
    static void tracePaths(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        long numberOfPaths,
        PatchProbabilityCallback birthProbabilityCallBack,
        PatchProbabilityCallback survivalProbabilityCallBack,
        ScorePathCallback scorePathCallBack,
        UpdatePatchCallback updateCallBack,
        const java::ArrayList<Patch *> *scenePatches);
};

#endif
