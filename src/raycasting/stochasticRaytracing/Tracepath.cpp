/**
Random walk generation
*/

#include <cstdlib>

#include "java/lang/System.h"

#ifdef RAYTRACING_ENABLED


#include "java/util/ArrayList.txx"
#include "common/Error.h"
#include "raycasting/stochasticRaytracing/McradP.h"
#include "raycasting/stochasticRaytracing/Tracepath.h"
#include "raycasting/stochasticRaytracing/Localline.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

double (*Tracepath::birthProbability)(const Patch *) = nullptr;
double Tracepath::sumProbabilities = 0.0;

/**
Initialises numberOfNodes, nodes allocated to zero and 'nodes' to the nullptr pointer
*/
void
Tracepath::initPath(Path *path) {
    path->numberOfNodes = path->nodesAllocated = 0;
    path->nodes = nullptr;
}

/**
Sets numberOfNodes to zero (forgets old path, but does not free the memory for the nodes
*/
void
Tracepath::clearPath(Path *path) {
    path->numberOfNodes = 0;
}

/**
Adds a node to the path. Re-allocates more space for the nodes if necessary
*/
void
Tracepath::pathAddNode(Path *path, Patch *patch, double prob, Vector3D inPoint, Vector3D outpoint) {
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
void
Tracepath::freePathNodes(Path *path) {
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
Path, previously initialised by initPath(). If required, photonMapTracePath()
allocates extra space for storing nodes calls to pathAddNode().
freePathNodes() should be called in order to dispose of this memory
when no longer needed
*/
Path *
Tracepath::tracePath(
    const VoxelGrid * sceneWorldVoxelGrid,
    Patch *origin,
    double birth_prob,
    double (*survivalProbabilityCallBack)(const Patch *P),
    Path *path)
{
    Vector3D inPoint = {0.0, 0.0, 0.0};
    Vector3D outpoint = {0.0, 0.0, 0.0};
    Patch *P = origin;
    double survivalProb;
    Ray ray;
    const RayHit *hit;
    RayHit hitStore;

    StochasticRelaxation::activeState().tracedPaths++;
    clearPath(path);
    pathAddNode(path, origin, birth_prob, inPoint, outpoint);
    do {
        StochasticRelaxation::activeState().tracedRays++;
        ray = Localline::mcrGenerateLocalLine(P, Sample4d::sample4D(static_cast<unsigned int>(McradP::topLevelStochasticRadiosityElement(P)->rayIndex)));
        McradP::topLevelStochasticRadiosityElement(P)->rayIndex++;
        if ( path->numberOfNodes > 1 && StochasticRelaxation::activeState().continuousRandomWalk ) {
            // Scattered ray originates at point of incidence of previous ray
            ray.position = path->nodes[path->numberOfNodes - 1].inPoint;
        }
        path->nodes[path->numberOfNodes - 1].outpoint = ray.position;

        hit = Localline::mcrShootRay(sceneWorldVoxelGrid, P, &ray, &hitStore);
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

double
Tracepath::patchNormalisedBirthProbability(const Patch *P) {
    return birthProbability(P) / sumProbabilities;
}

/**
Traces 'numberOfPaths' paths with given birth probabilities
*/
void
Tracepath::tracePaths(
    const VoxelGrid *sceneWorldVoxelGrid,
    long numberOfPaths,
    double (*birthProbabilityCallBack)(const Patch *P),
    double (*survivalProbabilityCallBack)(const Patch *P),
    void (*scorePathCallBack)(const Path *, long numberOfPaths, double (*birthProb)(const Patch *)),
    void (*updateCallBack)(const Patch *P, double w),
    const java::ArrayList<Patch *> *scenePatches)
{
    double rnd;
    double pCumulative;
    long pathCount;
    Path path{};

    StochasticRelaxation::activeState().prevTracedRays = StochasticRelaxation::activeState().tracedRays;
    birthProbability = birthProbabilityCallBack;

    // Compute sampling probability normalisation factor
    sumProbabilities = 0.0;
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        const Patch *patch = scenePatches->get(i);
        sumProbabilities += birthProbabilityCallBack(patch);
        Coefficientsmcrad::stochasticRadiosityClearCoefficients(McradP::getTopLevelPatchReceivedRad(patch), McradP::getTopLevelPatchBasis(patch));
    }
    if ( sumProbabilities < Numeric::EPSILON ) {
        Error::warning("tracePaths", "No sources");
        return;
    }

    // Fire off paths from the patches, propagate radiance
    initPath(&path);
    rnd = drand48();
    pathCount = 0;
    pCumulative = 0.0;
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        double p = birthProbabilityCallBack(patch) / sumProbabilities;
        long paths_this_patch = static_cast<int>(java::Math::floor((pCumulative + p) * static_cast<double>(numberOfPaths) + rnd)) - pathCount;
        for ( int j = 0; j < paths_this_patch; j++ ) {
            tracePath(sceneWorldVoxelGrid, patch, p, survivalProbabilityCallBack, &path);
            scorePathCallBack(&path, numberOfPaths, patchNormalisedBirthProbability);
        }
        pCumulative += p;
        pathCount += paths_this_patch;
    }

    java::System::err.printf("\n");
    freePathNodes(&path);

    // updateCallBack radiance, compute new total and un-shot flux
    StochasticRelaxation::activeState().unShotFlux.clear();
    StochasticRelaxation::activeState().unShotYmp = 0.0;
    StochasticRelaxation::activeState().totalFlux.clear();
    StochasticRelaxation::activeState().totalYmp = 0.0;

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        const Patch *patch = scenePatches->get(i);
        updateCallBack(patch, static_cast<double>(numberOfPaths) / sumProbabilities);
        StochasticRelaxation::activeState().unShotFlux.addScaled(
            StochasticRelaxation::activeState().unShotFlux,
            static_cast<float>(M_PI) * patch->area,
            McradP::getTopLevelPatchUnShotRad(patch)[0]);
        StochasticRelaxation::activeState().totalFlux.addScaled(
            StochasticRelaxation::activeState().totalFlux,
            static_cast<float>(M_PI) * patch->area,
            McradP::getTopLevelPatchRad(patch)[0]);
    }
}

#endif
