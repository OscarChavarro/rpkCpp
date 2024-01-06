/*
random walk generation
*/

#include <cstdlib>

#include "common/error.h"
#include "material/statistics.h"
#include "scene/scene.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/tracepath.h"
#include "raycasting/stochasticRaytracing/localline.h"

void InitPath(PATH *path) {
    path->nrnodes = path->nodesalloced = 0;
    path->nodes = (PATHNODE *) nullptr;
}

void ClearPath(PATH *path) {
    path->nrnodes = 0;
}

void PathAddNode(PATH *path, PATCH *patch, double prob, Vector3D inpoint, Vector3D outpoint) {
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

void FreePathNodes(PATH *path) {
    if ( path->nodes ) {
        free((char *) path->nodes);
    }
    path->nodesalloced = 0;
    path->nodes = (PATHNODE *) nullptr;
}

/* path nodes are filled in 'path', 'path' itself is returned. */
PATH *TracePath(PATCH *origin,
                double birth_prob,
                double (*SurvivalProbability)(PATCH *P),
                PATH *path) {
    Vector3D inpoint = {0., 0., 0.}, outpoint = {0., 0., 0.};
    PATCH *P = origin;
    double surv_prob;
    Ray ray;
    HITREC *hit, hitstore;

    mcr.traced_paths++;
    ClearPath(path);
    PathAddNode(path, origin, birth_prob, inpoint, outpoint);
    do {
        mcr.traced_rays++;
        ray = McrGenerateLocalLine(P, Sample4D(RAY_INDEX(P)++));
        if ( path->nrnodes > 1 && mcr.continuous_random_walk ) {
            /* scattered ray originates at point of incidence of previous ray */
            ray.pos = path->nodes[path->nrnodes - 1].inpoint;
        }
        path->nodes[path->nrnodes - 1].outpoint = ray.pos;

        hit = McrShootRay(P, &ray, &hitstore);
        if ( !hit ) {
            break;
        }    /* path disappears into background */

        P = hit->patch;
        surv_prob = SurvivalProbability(P);
        PathAddNode(path, P, surv_prob, hit->point, outpoint);
    } while ( drand48() < surv_prob );    /* repeat until absorption */

    return path;
}

static double (*birth_prob)(PATCH *), sum_probs;

static double PatchNormalisedBirthProbability(PATCH *P) {
    return birth_prob(P) / sum_probs;
}

/* traces 'nr_paths' paths with given birth probabilities */
void TracePaths(long nr_paths,
                double (*BirthProbability)(PATCH *P),
                double (*SurvivalProbability)(PATCH *P),
                void (*ScorePath)(PATH *, long nr_paths, double (*birth_prob)(PATCH *)),
                void (*Update)(PATCH *P, double w)) {
    double rnd, p_cumul;
    long path_count;
    PATH path;

    mcr.prev_traced_rays = mcr.traced_rays;
    birth_prob = BirthProbability;

    /* compute sampling probability normalisation factor */
    sum_probs = 0.;
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    sum_probs += BirthProbability(P);
                    CLEARCOEFFICIENTS(RECEIVED_RAD(P), BAS(P));
                }
    EndForAll;
    if ( sum_probs < EPSILON ) {
        Warning("TracePaths", "No sources");
        return;
    }

    /* fire off paths from the patches, propagate radiance */
    InitPath(&path);
    rnd = drand48();
    path_count = 0;
    p_cumul = 0.;
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    double p = BirthProbability(P) / sum_probs;
                    long i, paths_this_patch = (int) floor((p_cumul + p) * (double) nr_paths + rnd) - path_count;
                    for ( i = 0; i < paths_this_patch; i++ ) {
                        TracePath(P, p, SurvivalProbability, &path);
                        ScorePath(&path, nr_paths, PatchNormalisedBirthProbability);
                    }
                    p_cumul += p;
                    path_count += paths_this_patch;
                }
    EndForAll;
    fprintf(stderr, "\n");
    FreePathNodes(&path);

    /* update radiance, compute new total and unshot flux. */
    colorClear(mcr.unshot_flux);
    mcr.unshot_ymp = 0.;
    colorClear(mcr.total_flux);
    mcr.total_ymp = 0.;
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    Update(P, (double) nr_paths / sum_probs);
                    colorAddScaled(mcr.unshot_flux, M_PI * P->area, UNSHOT_RAD(P)[0], mcr.unshot_flux);
                    colorAddScaled(mcr.total_flux, M_PI * P->area, RAD(P)[0], mcr.total_flux);
                }
    EndForAll;
}
