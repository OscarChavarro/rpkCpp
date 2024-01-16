#include <cstdio>

#include "common/error.h"
#include "material/statistics.h"
#include "scene/scene.h"
#include "raycasting/stochasticRaytracing/mcrad.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/tracepath.h"
#include "raycasting/stochasticRaytracing/stochjacobi.h"

static void
randomWalkRadiosityPrintPatchData(FILE *out, Patch *patch) {
    monteCarloRadiosityPrintElement(out, TOPLEVEL_ELEMENT(patch));
}

static void
randomWalkRadiosityInit() {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.method = RANDOM_WALK_RADIOSITY_METHOD;
    monteCarloRadiosityInit();
}

static void
randomWalkRadiosityPrintStats() {
    fprintf(stderr, "%g secs., total radiance rays = %ld",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays);
    fprintf(stderr, ", total flux = ");
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.print(stderr);
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
        fprintf(stderr, "\ntotal importance rays = %ld, total importance = %g, GLOBAL_statistics_totalArea = %g",
                GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp, GLOBAL_statistics_totalArea);
    }
    fprintf(stderr, "\n");
}

/**
Used as un-normalised probability for mimicking global lines
*/
static double
randomWalkRadiosityPatchArea(Patch *P) {
    return P->area;
}

/**
Probability proportional to power to be propagated
*/
static double
randomWalkRadiosityScalarSourcePower(Patch *P) {
    COLOR radiance = TOPLEVEL_ELEMENT(P)->source_rad;
    return /* M_PI * */ P->area * colorSumAbsComponents(radiance);
}

/**
Returns a double instead of a float in order to make it useful as
a survival probability function
*/
static double
randomWalkRadiosityScalarReflectance(Patch *P) {
    return monteCarloRadiosityScalarReflectance(P);
}

static COLOR *
randomWalkRadiosityGetSelfEmittedRadiance(StochasticRadiosityElement *elem) {
    static COLOR Ed[MAX_BASIS_SIZE];
    stochasticRadiosityClearCoefficients(Ed, elem->basis);
    Ed[0] = TOPLEVEL_ELEMENT(elem->patch)->Ed; // Emittance
    return Ed;
}

/**
Subtracts (1 - rho) * control radiosity from the source radiosity of each patch
*/
static void
randomWalkRadiosityReduceSource() {
    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        Patch *patch = window->patch;
        COLOR newSourceRadiance;
        COLOR rho;

        colorSetMonochrome(newSourceRadiance, 1.0);
        rho = TOPLEVEL_ELEMENT(patch)->Rd; // Reflectance
        colorSubtract(newSourceRadiance, rho, newSourceRadiance); // 1 - rho
        colorProduct(newSourceRadiance, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance,
                     newSourceRadiance); // (1-rho) * beta
        colorSubtract(TOPLEVEL_ELEMENT(patch)->source_rad, newSourceRadiance, newSourceRadiance); // E - (1-rho) * beta
        TOPLEVEL_ELEMENT(patch)->source_rad = newSourceRadiance;
    }
}

static double
randomWalkRadiosityScoreWeight(PATH *path, int n) {
    double w = 0.;
    int t = path->nrnodes - ((GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkNumLast > 0) ? GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkNumLast : 1);

    switch ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorKind ) {
        case RW_COLLISION:
            w = 1.;
            break;
        case RW_ABSORPTION:
            if ( n == path->nrnodes - 1 ) {    /* last node */
                w = 1. / (1. - path->nodes[n].probability);
            }
            break;
        case RW_SURVIVAL:
            if ( n < path->nrnodes - 1 ) {    /* not last node */
                w = 1. / path->nodes[n].probability;
            }
            break;
        case RW_LAST_BUT_NTH:
            if ( n == t - 1 ) {
                int i = path->nrnodes - 1;
                PATHNODE *node = &path->nodes[i];
                w = 1. / (1. - node->probability);    /* absorption prob of the last node */
                for ( i--, node--; i >= n; i--, node-- ) {
                    w /= node->probability;
                }       /* survival prob of n...numberOfNodes-2th node */
            }
            break;
        case RW_N_LAST:
            if ( n == t ) {
                /* 1/absorption probability of the last path node */
                w = 1. / (1. - path->nodes[path->nrnodes - 1].probability);
            } else if ( n > t ) {
                w = 1.;
            }
            break;
        default:
            logFatal(-1, "randomWalkRadiosityScoreWeight", "Unknown random walk estimator kind %d",
                     GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorKind);
    }
    return w;
}

static void
randomWalkRadiosityShootingScore(PATH *path, long nr_paths, double (*birth_prob)(Patch *)) {
    COLOR accum_pow;
    int n;
    PATHNODE *node = &path->nodes[0];

    /* path->nodes[0].probability is birth probability of the path */
    colorScale((node->patch->area / node->probability), TOPLEVEL_ELEMENT(node->patch)->source_rad, accum_pow);
    for ( n = 1, node++; n < path->nrnodes; n++, node++ ) {
        double uin = 0., vin = 0., uout = 0., vout = 0., r = 1., w;
        int i;
        Patch *P = node->patch;
        COLOR Rd = TOPLEVEL_ELEMENT(P)->Rd;
        colorProduct(accum_pow, Rd, accum_pow);

        patchUniformUv(P, &node->inpoint, &uin, &vin);
        if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
            r = 0.;
            if ( n < path->nrnodes - 1 ) {
                /* not continuous random walk and not node of absorption */
                patchUniformUv(P, &node->outpoint, &uout, &vout);
            }
        }

        w = randomWalkRadiosityScoreWeight(path, n);

        for ( i = 0; i < getTopLevelPatchBasis(P)->size; i++ ) {
            double dual = getTopLevelPatchBasis(P)->dualfunction[i](uin, vin) / P->area;
            colorAddScaled(getTopLevelPatchReceivedRad(P)[i], (w * dual / (double) nr_paths), accum_pow, getTopLevelPatchReceivedRad(P)[i]);

            if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
                double basf = getTopLevelPatchBasis(P)->function[i](uout, vout);
                r += dual * P->area * basf;
            }
        }

        colorScale((r / node->probability), accum_pow, accum_pow);
    }
}

static void
randomWalkRadiosityShootingUpdate(Patch *P, double w) {
    double k, old_quality;
    old_quality = TOPLEVEL_ELEMENT(P)->quality;
    TOPLEVEL_ELEMENT(P)->quality += w;
    if ( TOPLEVEL_ELEMENT(P)->quality < EPSILON ) {
        return;
    }
    k = old_quality / TOPLEVEL_ELEMENT(P)->quality;

    /* subtract self-emitted rad */
    colorSubtract(getTopLevelPatchRad(P)[0], TOPLEVEL_ELEMENT(P)->source_rad, getTopLevelPatchRad(P)[0]);

    /* weight with previous results */
    stochasticRadiosityScaleCoefficients((float)k, getTopLevelPatchRad(P), getTopLevelPatchBasis(P));
    stochasticRadiosityScaleCoefficients((1.0f - (float)k), getTopLevelPatchReceivedRad(P), getTopLevelPatchBasis(P));
    stochasticRadiosityAddCoefficients(getTopLevelPatchRad(P), getTopLevelPatchReceivedRad(P), getTopLevelPatchBasis(P));

    /* re-add self-emitted rad */
    colorAdd(getTopLevelPatchRad(P)[0], TOPLEVEL_ELEMENT(P)->source_rad, getTopLevelPatchRad(P)[0]);

    /* clear un-shot and received radiance */
    stochasticRadiosityClearCoefficients(getTopLevelPatchUnShotRad(P), getTopLevelPatchBasis(P));
    stochasticRadiosityClearCoefficients(getTopLevelPatchReceivedRad(P), getTopLevelPatchBasis(P));
}

static void
randomWalkRadiosityDoShootingIteration() {
    long nr_walks;

    nr_walks = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays;
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
        nr_walks *= approxdesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size;
    } else {
        nr_walks *= pow(approxdesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size, 1. / (1. -
                                                                                                                                  colorMaximumComponent(GLOBAL_statistics_averageReflectivity)));
    }

    fprintf(stderr, "Shooting iteration %d (%ld paths, approximately %ld rays)\n",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration,
            nr_walks, (long) floor((double) nr_walks / (1. -
                    colorMaximumComponent(GLOBAL_statistics_averageReflectivity))));

    tracePaths(nr_walks,
               randomWalkRadiosityScalarSourcePower, randomWalkRadiosityScalarReflectance,
               randomWalkRadiosityShootingScore,
               randomWalkRadiosityShootingUpdate);
}

/**
Determines control radiosity value for collision gathering estimator
*/
static COLOR
randomWalkRadiosityDetermineGatheringControlRadiosity() {
    COLOR c1;
    COLOR c2;
    COLOR cr;

    colorClear(c1);
    colorClear(c2);

    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        COLOR absorb;
        COLOR rho;
        COLOR Ed;
        COLOR num;
        COLOR denom;
        Patch *patch = window->patch;

        colorSetMonochrome(absorb, 1.0);
        rho = TOPLEVEL_ELEMENT(patch)->Rd;
        colorSubtract(absorb, rho, absorb); // 1-rho

        Ed = TOPLEVEL_ELEMENT(patch)->source_rad;
        colorProduct(absorb, Ed, num);
        colorAddScaled(c1, patch->area, num, c1); // A_P (1-rho_P) E_P

        colorProduct(absorb, absorb, denom);
        colorAddScaled(c2, patch->area, denom, c2); // A_P (1-rho_P)^2
    }

    colorDivide(c1, c2, cr);
    fprintf(stderr, "Control radiosity value = ");
    cr.print(stderr);
    fprintf(stderr, ", luminosity = %g\n", colorLuminance(cr));

    return cr;
}

static void
randomWalkRadiosityCollisionGatheringScore(PATH *path, long nr_paths, double (*birth_prob)(Patch *)) {
    COLOR accum_rad;
    int n;
    PATHNODE *node = &path->nodes[path->nrnodes - 1];
    accum_rad = TOPLEVEL_ELEMENT(node->patch)->source_rad;
    for ( n = path->nrnodes - 2, node--; n >= 0; n--, node-- ) {
        double uin = 0., vin = 0., uout = 0., vout = 0., r = 1.;
        int i;
        Patch *P = node->patch;
        COLOR Rd = TOPLEVEL_ELEMENT(P)->Rd;
        colorProduct(Rd, accum_rad, accum_rad);

        patchUniformUv(P, &node->outpoint, &uout, &vout);
        if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
            r = 0.;
            if ( n > 0 ) {
                /* not continuous random walk and not birth node */
                patchUniformUv(P, &node->inpoint, &uin, &vin);
            }
        }

        for ( i = 0; i < getTopLevelPatchBasis(P)->size; i++ ) {
            double dual = getTopLevelPatchBasis(P)->dualfunction[i](uout, vout);    /* = dual basis f * area */
            colorAddScaled(getTopLevelPatchReceivedRad(P)[i], dual, accum_rad, getTopLevelPatchReceivedRad(P)[i]);

            if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
                double basf = getTopLevelPatchBasis(P)->function[i](uin, vin);
                r += basf * dual;
            }
        }
        TOPLEVEL_ELEMENT(P)->ng++;

        colorScale((r / node->probability), accum_rad, accum_rad);
        colorAdd(accum_rad, TOPLEVEL_ELEMENT(P)->source_rad, accum_rad);
    }
}

static void
randomWalkRadiosityGatheringUpdate(Patch *P, double w) {
    /* use un-shot rad for accumulating sum of contributions */
    stochasticRadiosityAddCoefficients(getTopLevelPatchUnShotRad(P), getTopLevelPatchReceivedRad(P),
                                       getTopLevelPatchBasis(P));
    stochasticRadiosityCopyCoefficients(getTopLevelPatchRad(P), getTopLevelPatchUnShotRad(P), getTopLevelPatchBasis(P));

    /* divide by nr of samples */
    if ( TOPLEVEL_ELEMENT(P)->ng > 0 )
        stochasticRadiosityScaleCoefficients((1.0f / (float)TOPLEVEL_ELEMENT(P)->ng), getTopLevelPatchRad(P), getTopLevelPatchBasis(P));

    /* add source radiance (source term estimation suppresion!) */
    colorAdd(getTopLevelPatchRad(P)[0], TOPLEVEL_ELEMENT(P)->source_rad, getTopLevelPatchRad(P)[0]);

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate ) {
        /* add constant control radiosity value */
        COLOR cr = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance;
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly ) {
            COLOR Rd = TOPLEVEL_ELEMENT(P)->Rd;
            colorProduct(Rd, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance, cr);
        }
        colorAdd(getTopLevelPatchRad(P)[0], cr, getTopLevelPatchRad(P)[0]);
    }

    stochasticRadiosityClearCoefficients(getTopLevelPatchReceivedRad(P), getTopLevelPatchBasis(P));
}

static void
randomWalkRadiosityDoGatheringIteration() {
    long nr_walks = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays;
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
        nr_walks *= approxdesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size;
    } else {
        nr_walks *= pow(approxdesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size, 1. / (1. -
                                                                                                                                  colorMaximumComponent(GLOBAL_statistics_averageReflectivity)));
    }

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate && GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration == 1 ) {
        /* constant control variate for gathering random walk radiosity */
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance = randomWalkRadiosityDetermineGatheringControlRadiosity();
        randomWalkRadiosityReduceSource();    /* do this only once!! */
    }

    fprintf(stderr, "Collision gathering iteration %d (%ld paths, approximately %ld rays)\n",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration,
            nr_walks, (long) floor((double) nr_walks / (1. -
                    colorMaximumComponent(GLOBAL_statistics_averageReflectivity))));

    tracePaths(nr_walks,
               randomWalkRadiosityPatchArea, randomWalkRadiosityScalarReflectance,
               randomWalkRadiosityCollisionGatheringScore,
               randomWalkRadiosityGatheringUpdate);
}

static void
randomWalkRadiosityUpdateSourceIllum(StochasticRadiosityElement *elem, double w) {
    stochasticRadiosityCopyCoefficients(elem->rad, elem->received_rad, elem->basis);
    elem->source_rad = elem->received_rad[0];
    stochasticRadiosityClearCoefficients(elem->unshot_rad, elem->basis);
    stochasticRadiosityClearCoefficients(elem->received_rad, elem->basis);
}

static void
randomWalkRadiosityDoFirstShot() {
    long nr_rays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays * approxdesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size;
    fprintf(stderr, "First shot (%ld rays):\n", nr_rays);
    DoStochasticJacobiIteration(nr_rays, randomWalkRadiosityGetSelfEmittedRadiance, nullptr,
                                randomWalkRadiosityUpdateSourceIllum);
    randomWalkRadiosityPrintStats();
}

static int
randomWalkRadiosityDoStep() {
    monteCarloRadiosityPreStep();

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration == 1 ) {
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly ) {
            randomWalkRadiosityDoFirstShot();
        }
    }

    switch ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorType ) {
        case RW_SHOOTING:
            randomWalkRadiosityDoShootingIteration();
            break;
        case RW_GATHERING:
            randomWalkRadiosityDoGatheringIteration();
            break;
        default:
            logFatal(-1, "randomWalkRadiosityDoStep", "Unknown random walk estimator type %d",
                     GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorType);
    }

    for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
        monteCarloRadiosityPatchComputeNewColor(window->patch);
    }

    return false; // Never converged
}

static void
randomWalkRadiosityTerminate() {
    /*  TerminateLightSampling(); */
    monteCarloRadiosityTerminate();
}

static char *
randomWalkRadiosityGetStats() {
    static char stats[2000];
    char *p;
    int n;

    p = stats;
    sprintf(p, "Random Walk Radiosity\nStatistics\n\n%n", &n);
    p += n;
    sprintf(p, "Iteration nr: %d\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration, &n);
    p += n;
    sprintf(p, "CPU time: %g secs\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, &n);
    p += n;
    sprintf(p, "Radiance rays: %ld\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays, &n);
    p += n;
    sprintf(p, "Importance rays: %ld\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays, &n);
    p += n;

    return stats;
}

RADIANCEMETHOD GLOBAL_stochasticRaytracing_randomWalkRadiosity = {
    "RandomWalk",
    3,
    "Random Walk Radiosity",
    monteCarloRadiosityDefaults,
    randomWalkRadiosityParseOptions,
    randomWalkRadiosityPrintOptions,
    randomWalkRadiosityInit,
    randomWalkRadiosityDoStep,
    randomWalkRadiosityTerminate,
    monteCarloRadiosityGetRadiance,
    monteCarloRadiosityCreatePatchData,
    randomWalkRadiosityPrintPatchData,
    monteCarloRadiosityDestroyPatchData,
    randomWalkRadiosityGetStats,
    (void (*)()) nullptr,
    monteCarloRadiosityRecomputeDisplayColors,
    monteCarloRadiosityUpdateMaterial,
    (void (*)(FILE *)) nullptr
};
