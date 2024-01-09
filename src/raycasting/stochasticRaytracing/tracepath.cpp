/**
Random walk generation
*/

#include <cstdlib>

#include "common/error.h"
#include "material/statistics.h"
#include "scene/scene.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/tracepath.h"
#include "raycasting/stochasticRaytracing/localline.h"

static double (*globalBirthProbability)(Patch *);
static double globalSumProbabilities;

/**
Initialises nrnodes, nodesalloced to zero and 'nodes' to the nullptr pointer
*/
static void
initPath(PATH *path) {
    path->nrnodes = path->nodesalloced = 0;
    path->nodes = (PATHNODE *) nullptr;
}

/**
Sets nrnodes to zero (forgets old path, but does not free the memory for the nodes
*/
static void
clearPath(PATH *path) {
    path->nrnodes = 0;
}

/**
Adds a node to the path. Re-allocates more space for the nodes if necessary
*/
static void
pathAddNode(PATH *path, Patch *patch, double prob, Vector3D inpoint, Vector3D outpoint) {
    PATHNODE *node;

    if ( path->nrnodes >= path->nodesalloced ) {
        PATHNODE *newnodes = (PATHNODE *)malloc((path->nodesalloced + 20) * sizeof(PATHNODE));
        if ( path->nodesalloced > 0 ) {
            int i;
            for ( i = 0; i < path->nrnodes; i++ ) {
                newnodes[i] = path->nodes[i];
            }    /* copy nodes */
            free((char *) path->nodes);
        }
        path->nodes = newnodes;
        path->nodesalloced += 20;
    }

    node = &path->nodes[path->nrnodes];
    node->patch = patch;
    node->probability = prob;
    node->inpoint = inpoint;
    node->outpoint = outpoint;
    path->nrnodes++;
}

/**
Disposes of the memory for storing path nodes
*/
static void
freePathNodes(PATH *path) {
    if ( path->nodes ) {
        free((char *) path->nodes);
    }
    path->nodesalloced = 0;
    path->nodes = (PATHNODE *) nullptr;
}

/**
Path nodes are filled in 'path', 'path' itself is returned

Traces a random walk originating at 'origin', with birth probability
'globalBirthProbability' (filled in as probability of the origin node: source term
estimation is being suppressed --- survival probability at the origin is
1). Survivial probability at other nodes than the origin is calculated by
'SurvivalProbability()', results are stored in 'path', which should be an
PATH, previously initialised by initPath(). If required, photonMapTracePath()
allocates extra space for storing nodes calls to pathAddNode().
freePathNodes() should be called in order to dispose of this memory
when no longer needed
*/
static PATH *
tracePath(
        Patch *origin,
        double birth_prob,
        double (*SurvivalProbability)(Patch *P),
        PATH *path)
{
    Vector3D inPoint = {0.0, 0.0, 0.0};
    Vector3D outpoint = {0.0, 0.0, 0.0};
    Patch *P = origin;
    double survProb;
    Ray ray;
    RayHit *hit;
    RayHit hitStore;

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedPaths++;
    clearPath(path);
    pathAddNode(path, origin, birth_prob, inPoint, outpoint);
    do {
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays++;
        ray = McrGenerateLocalLine(P, Sample4D(TOPLEVEL_ELEMENT(P)->ray_index++));
        if ( path->nrnodes > 1 && GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
            // Scattered ray originates at point of incidence of previous ray
            ray.pos = path->nodes[path->nrnodes - 1].inpoint;
        }
        path->nodes[path->nrnodes - 1].outpoint = ray.pos;

        hit = McrShootRay(P, &ray, &hitStore);
        if ( !hit ) {
            // Path disappears into background
            break;
        }

        P = hit->patch;
        survProb = SurvivalProbability(P);
        pathAddNode(path, P, survProb, hit->point, outpoint);
    } while ( drand48() < survProb ); // Repeat until absorption

    return path;
}

static double
patchNormalisedBirthProbability(Patch *P) {
    return globalBirthProbability(P) / globalSumProbabilities;
}

/**
Traces 'numberOfPaths' paths with given birth probabilities
*/
void
tracePaths(
    long numberOfPaths,
    double (*BirthProbability)(Patch *P),
    double (*SurvivalProbability)(Patch *P),
    void (*ScorePath)(PATH *, long nr_paths, double (*birth_prob)(Patch *)),
    void (*Update)(Patch *P, double w))
{
    double rnd;
    double pCumul;
    long path_count;
    PATH path{};

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevTracedRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays;
    globalBirthProbability = BirthProbability;

    // Compute sampling probability normalisation factor
    globalSumProbabilities = 0.;
    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        globalSumProbabilities += BirthProbability(window->patch);
        stochasticRadiosityClearCoefficients(getTopLevelPatchReceivedRad(window->patch), getTopLevelPatchBasis(window->patch));
    }
    if ( globalSumProbabilities < EPSILON ) {
        logWarning("tracePaths", "No sources");
        return;
    }

    // Fire off paths from the patches, propagate radiance
    initPath(&path);
    rnd = drand48();
    path_count = 0;
    pCumul = 0.0;
    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        double p = BirthProbability(window->patch) / globalSumProbabilities;
        long i, paths_this_patch = (int) floor((pCumul + p) * (double) numberOfPaths + rnd) - path_count;
        for ( i = 0; i < paths_this_patch; i++ ) {
            tracePath(window->patch, p, SurvivalProbability, &path);
            ScorePath(&path, numberOfPaths, patchNormalisedBirthProbability);
        }
        pCumul += p;
        path_count += paths_this_patch;
    }

    fprintf(stderr, "\n");
    freePathNodes(&path);

    // Update radiance, compute new total and un-shot flux
    colorClear(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp = 0.0;
    colorClear(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp = 0.0;

    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        Patch *patch = window->patch;
        Update(patch, (double) numberOfPaths / globalSumProbabilities);
        colorAddScaled(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux, M_PI * patch->area,
                       getTopLevelPatchUnShotRad(patch)[0],
                       GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux);
        colorAddScaled(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux, M_PI * patch->area,
                       getTopLevelPatchRad(patch)[0], GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux);
    }
}
