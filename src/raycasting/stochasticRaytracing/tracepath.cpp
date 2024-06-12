/**
Random walk generation
*/

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include <cstdlib>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/tracepath.h"
#include "raycasting/stochasticRaytracing/localline.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

static double (*globalBirthProbability)(const Patch *);
static double globalSumProbabilities;

StochasticRaytracingPathNode::StochasticRaytracingPathNode(): patch(), probability(), inPoint(), outpoint() {}

/**
Initialises numberOfNodes, nodes allocated to zero and 'nodes' to the nullptr pointer
*/
static void
initPath(PATH *path) {
    path->numberOfNodes = path->nodesAllocated = 0;
    path->nodes = nullptr;
}

/**
Sets numberOfNodes to zero (forgets old path, but does not free the memory for the nodes
*/
static void
clearPath(PATH *path) {
    path->numberOfNodes = 0;
}

/**
Adds a node to the path. Re-allocates more space for the nodes if necessary
*/
static void
pathAddNode(PATH *path, Patch *patch, double prob, Vector3D inPoint, Vector3D outpoint) {
    StochasticRaytracingPathNode *node;

    if ( path->numberOfNodes >= path->nodesAllocated ) {
        StochasticRaytracingPathNode *newNodes = new StochasticRaytracingPathNode[path->nodesAllocated + 20];
        if ( path->nodesAllocated > 0 ) {
            for ( int i = 0; i < path->numberOfNodes; i++ ) {
                // Copy nodes
                newNodes[i] = path->nodes[i];
            }
            delete[] path->nodes;
        }
        path->nodes = newNodes;
        path->nodesAllocated += 20;
    }

    node = &path->nodes[path->numberOfNodes];
    node->patch = patch;
    node->probability = prob;
    node->inPoint = inPoint;
    node->outpoint = outpoint;
    path->numberOfNodes++;
}

/**
Disposes of the memory for storing path nodes
*/
static void
freePathNodes(PATH *path) {
    if ( path->nodes != nullptr ) {
        delete[] path->nodes;
        path->nodes = nullptr;
    }
    path->nodesAllocated = 0;
}

/**
Path nodes are filled in 'path', 'path' itself is returned

Traces a random walk originating at 'origin', with birth stochasticJacobiProbability
'globalBirthProbability' (filled in as stochasticJacobiProbability of the origin node: source term
estimation is being suppressed --- survival stochasticJacobiProbability at the origin is
1). Survival stochasticJacobiProbability at other nodes than the origin is calculated by
'survivalProbabilityCallBack()', results are stored in 'path', which should be an
PATH, previously initialised by initPath(). If required, photonMapTracePath()
allocates extra space for storing nodes calls to pathAddNode().
freePathNodes() should be called in order to dispose of this memory
when no longer needed
*/
static PATH *
tracePath(
    const VoxelGrid * sceneWorldVoxelGrid,
    Patch *origin,
    double birth_prob,
    double (*survivalProbabilityCallBack)(const Patch *P),
    PATH *path)
{
    Vector3D inPoint = {0.0, 0.0, 0.0};
    Vector3D outpoint = {0.0, 0.0, 0.0};
    Patch *P = origin;
    double survivalProb;
    Ray ray;
    const RayHit *hit;
    RayHit hitStore;

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedPaths++;
    clearPath(path);
    pathAddNode(path, origin, birth_prob, inPoint, outpoint);
    do {
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays++;
        ray = mcrGenerateLocalLine(P, sample4D((unsigned int)topLevelStochasticRadiosityElement(P)->rayIndex));
        topLevelStochasticRadiosityElement(P)->rayIndex++;
        if ( path->numberOfNodes > 1 && GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
            // Scattered ray originates at point of incidence of previous ray
            ray.pos = path->nodes[path->numberOfNodes - 1].inPoint;
        }
        path->nodes[path->numberOfNodes - 1].outpoint = ray.pos;

        hit = mcrShootRay(sceneWorldVoxelGrid, P, &ray, &hitStore);
        if ( !hit ) {
            // Path disappears into background
            break;
        }

        P = hit->getPatch();
        survivalProb = survivalProbabilityCallBack(P);
        pathAddNode(path, P, survivalProb, hit->getPoint(), outpoint);
    } while ( drand48() < survivalProb ); // Repeat until absorption

    return path;
}

static double
patchNormalisedBirthProbability(const Patch *P) {
    return globalBirthProbability(P) / globalSumProbabilities;
}

/**
Traces 'numberOfPaths' paths with given birth probabilities
*/
void
tracePaths(
    const VoxelGrid *sceneWorldVoxelGrid,
    long numberOfPaths,
    double (*birthProbabilityCallBack)(const Patch *P),
    double (*survivalProbabilityCallBack)(const Patch *P),
    void (*scorePathCallBack)(const PATH *, long numberOfPaths, double (*birthProb)(const Patch *)),
    void (*updateCallBack)(const Patch *P, double w),
    const java::ArrayList<Patch *> *scenePatches)
{
    double rnd;
    double pCumulative;
    long pathCount;
    PATH path{};

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevTracedRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays;
    globalBirthProbability = birthProbabilityCallBack;

    // Compute sampling probability normalisation factor
    globalSumProbabilities = 0.0;
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        const Patch *patch = scenePatches->get(i);
        globalSumProbabilities += birthProbabilityCallBack(patch);
        stochasticRadiosityClearCoefficients(getTopLevelPatchReceivedRad(patch), getTopLevelPatchBasis(patch));
    }
    if ( globalSumProbabilities < Numeric::EPSILON ) {
        logWarning("tracePaths", "No sources");
        return;
    }

    // Fire off paths from the patches, propagate radiance
    initPath(&path);
    rnd = drand48();
    pathCount = 0;
    pCumulative = 0.0;
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        double p = birthProbabilityCallBack(patch) / globalSumProbabilities;
        long paths_this_patch = (int)java::Math::floor((pCumulative + p) * (double) numberOfPaths + rnd) - pathCount;
        for ( int j = 0; j < paths_this_patch; j++ ) {
            tracePath(sceneWorldVoxelGrid, patch, p, survivalProbabilityCallBack, &path);
            scorePathCallBack(&path, numberOfPaths, patchNormalisedBirthProbability);
        }
        pCumulative += p;
        pathCount += paths_this_patch;
    }

    fprintf(stderr, "\n");
    freePathNodes(&path);

    // updateCallBack radiance, compute new total and un-shot flux
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.clear();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.clear();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp = 0.0;

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        const Patch *patch = scenePatches->get(i);
        updateCallBack(patch, (double) numberOfPaths / globalSumProbabilities);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.addScaled(
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux,
            (float)M_PI * patch->area,
            getTopLevelPatchUnShotRad(patch)[0]);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.addScaled(
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux,
            (float)M_PI * patch->area,
            getTopLevelPatchRad(patch)[0]);
    }
}

#endif
