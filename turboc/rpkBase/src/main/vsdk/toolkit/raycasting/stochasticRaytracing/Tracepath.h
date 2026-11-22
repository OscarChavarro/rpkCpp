/**
Random walk generation
*/

#ifndef __TRACE_PATH__
#define __TRACE_PATH__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRaytracingPathNode.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Path.h"

class Tracepath{ public:
    typedef double (*PatchProbabilityCallback)(const Patch *patch);
    typedef void (*ScorePathCallback)(const Path *, long numberOfPaths, PatchProbabilityCallback birthProb);
    typedef void (*UpdatePatchCallback)(const Patch *patch, double w);

  private:
    static PatchProbabilityCallback birthProbability;
    static double sumProbabilities;

    static void initPath(Path *path);
    static void clearPath(Path *path);
    static void pathAddNode(Path *path, Patch *patch, double probability, Vector3Df inPoint, Vector3Df outpoint);
    static void freePathNodes(Path *path);
    static Path *tracePath( const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, Patch *origin, double birthProb, PatchProbabilityCallback survivalProbabilityCallBack, Path *path);
    static double patchNormalisedBirthProbability(const Patch *patch);

  public:
    static void tracePaths( const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, long numberOfPaths, PatchProbabilityCallback birthProbabilityCallBack, PatchProbabilityCallback survivalProbabilityCallBack, ScorePathCallback scorePathCallBack, UpdatePatchCallback updateCallBack, const ArrayList<Patch *> *scenePatches);
};

#endif
