#include <cstdio>

#include "common/error.h"
#include "material/statistics.h"
#include "raycasting/stochasticRaytracing/mcrad.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/tracepath.h"
#include "raycasting/stochasticRaytracing/stochjacobi.h"
#include "raycasting/stochasticRaytracing/RandomWalkRadianceMethod.h"

RandomWalkRadianceMethod::RandomWalkRadianceMethod() {
    monteCarloRadiosityDefaults();
    className = RANDOM_WALK;
}

RandomWalkRadianceMethod::~RandomWalkRadianceMethod() {
}

void
RandomWalkRadianceMethod::parseOptions(int *argc, char **argv) {
    randomWalkRadiosityParseOptions(argc, argv);
}

COLOR
RandomWalkRadianceMethod::getRadiance(Patch *patch, double u, double v, Vector3D dir) {
    return monteCarloRadiosityGetRadiance(patch, u, v, dir);
}

Element *
RandomWalkRadianceMethod::createPatchData(Patch *patch) {
    return monteCarloRadiosityCreatePatchData(patch);
}

void
RandomWalkRadianceMethod::destroyPatchData(Patch *patch) {
    monteCarloRadiosityDestroyPatchData(patch);
}

void
RandomWalkRadianceMethod::renderScene(java::ArrayList<Patch *> *scenePatches) {
}

void
RandomWalkRadianceMethod::writeVRML(FILE *fp){
}

void
RandomWalkRadianceMethod::initialize(java::ArrayList<Patch *> *scenePatches) {
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
                GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp, GLOBAL_statistics.totalArea);
    }
    fprintf(stderr, "\n");
}

/**
Used as un-normalised stochasticJacobiProbability for mimicking global lines
*/
static double
randomWalkRadiosityPatchArea(Patch *P) {
    return P->area;
}

/**
stochasticJacobiProbability proportional to power to be propagated
*/
static double
randomWalkRadiosityScalarSourcePower(Patch *P) {
    COLOR radiance = topLevelGalerkinElement(P)->sourceRad;
    return /* M_PI * */ P->area * colorSumAbsComponents(radiance);
}

/**
Returns a double instead of a float in order to make it useful as
a survival stochasticJacobiProbability function
*/
static double
randomWalkRadiosityScalarReflectance(Patch *P) {
    return monteCarloRadiosityScalarReflectance(P);
}

static COLOR *
randomWalkRadiosityGetSelfEmittedRadiance(StochasticRadiosityElement *elem) {
    static COLOR Ed[MAX_BASIS_SIZE];
    stochasticRadiosityClearCoefficients(Ed, elem->basis);
    Ed[0] = topLevelGalerkinElement(elem->patch)->Ed; // Emittance
    return Ed;
}

/**
Subtracts (1 - rho) * control radiosity from the source radiosity of each patch
*/
static void
randomWalkRadiosityReduceSource(java::ArrayList<Patch *> *scenePatches) {
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        COLOR newSourceRadiance;
        COLOR rho;

        colorSetMonochrome(newSourceRadiance, 1.0);
        rho = topLevelGalerkinElement(patch)->Rd; // Reflectance
        colorSubtract(newSourceRadiance, rho, newSourceRadiance); // 1 - rho
        colorProduct(newSourceRadiance, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance,
                     newSourceRadiance); // (1-rho) * beta
        colorSubtract(topLevelGalerkinElement(patch)->sourceRad, newSourceRadiance, newSourceRadiance); // E - (1-rho) * beta
        topLevelGalerkinElement(patch)->sourceRad = newSourceRadiance;
    }
}

static double
randomWalkRadiosityScoreWeight(PATH *path, int n) {
    double w = 0.0;
    int t = path->numberOfNodes - ((GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkNumLast > 0) ? GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkNumLast : 1);

    switch ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorKind ) {
        case RW_COLLISION:
            w = 1.0;
            break;
        case RW_ABSORPTION:
            if ( n == path->numberOfNodes - 1 ) {
                // Last node
                w = 1.0 / (1.0 - path->nodes[n].probability);
            }
            break;
        case RW_SURVIVAL:
            if ( n < path->numberOfNodes - 1 ) {
                // Not last node
                w = 1.0 / path->nodes[n].probability;
            }
            break;
        case RW_LAST_BUT_NTH:
            if ( n == t - 1 ) {
                int i = path->numberOfNodes - 1;
                StochasticRaytracingPathNode *node = &path->nodes[i];
                w = 1.0 / (1.0 - node->probability);
                // Absorption prob of the last node
                for ( i--, node--; i >= n; i--, node-- ) {
                    // Survival prob of n...numberOfNodes-2th node
                    w /= node->probability;
                }
            }
            break;
        case RW_N_LAST:
            if ( n == t ) {
                // 1/absorption probability of the last path node
                w = 1.0 / (1.0 - path->nodes[path->numberOfNodes - 1].probability);
            } else if ( n > t ) {
                w = 1.0;
            }
            break;
        default:
            logFatal(-1, "randomWalkRadiosityScoreWeight", "Unknown random walk estimator kind %d",
                     GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorKind);
    }
    return w;
}

static void
randomWalkRadiosityShootingScore(PATH *path, long nr_paths, double (* /*birthProb*/)(Patch *)) {
    COLOR accumPow;
    int n;
    StochasticRaytracingPathNode *node = &path->nodes[0];

    /* path->nodes[0].probability is birth probability of the path */
    colorScale((float)(node->patch->area / node->probability), topLevelGalerkinElement(node->patch)->sourceRad, accumPow);
    for ( n = 1, node++; n < path->numberOfNodes; n++, node++ ) {
        double uin = 0.0;
        double vin = 0.0;
        double uOut = 0.0;
        double vOut = 0.0;
        double r = 1.0;
        double w;
        int i;
        Patch *P = node->patch;
        COLOR Rd = topLevelGalerkinElement(P)->Rd;
        colorProduct(accumPow, Rd, accumPow);

        P->uniformUv(&node->inPoint, &uin, &vin);
        if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
            r = 0.0;
            if ( n < path->numberOfNodes - 1 ) {
                // Not continuous random walk and not node of absorption
                P->uniformUv(&node->outpoint, &uOut, &vOut);
            }
        }

        w = randomWalkRadiosityScoreWeight(path, n);

        for ( i = 0; i < getTopLevelPatchBasis(P)->size; i++ ) {
            double dual = getTopLevelPatchBasis(P)->dualFunction[i](uin, vin) / P->area;
            colorAddScaled(getTopLevelPatchReceivedRad(P)[i], (float)(w * dual / (double) nr_paths), accumPow, getTopLevelPatchReceivedRad(P)[i]);

            if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
                double basf = getTopLevelPatchBasis(P)->function[i](uOut, vOut);
                r += dual * P->area * basf;
            }
        }

        colorScale((float)(r / node->probability), accumPow, accumPow);
    }
}

static void
randomWalkRadiosityShootingUpdate(Patch *P, double w) {
    double k, old_quality;
    old_quality = topLevelGalerkinElement(P)->quality;
    topLevelGalerkinElement(P)->quality += (float)w;
    if ( topLevelGalerkinElement(P)->quality < EPSILON ) {
        return;
    }
    k = old_quality / topLevelGalerkinElement(P)->quality;

    /* subtract self-emitted rad */
    colorSubtract(getTopLevelPatchRad(P)[0], topLevelGalerkinElement(P)->sourceRad, getTopLevelPatchRad(P)[0]);

    /* weight with previous results */
    stochasticRadiosityScaleCoefficients((float)k, getTopLevelPatchRad(P), getTopLevelPatchBasis(P));
    stochasticRadiosityScaleCoefficients((1.0f - (float)k), getTopLevelPatchReceivedRad(P), getTopLevelPatchBasis(P));
    stochasticRadiosityAddCoefficients(getTopLevelPatchRad(P), getTopLevelPatchReceivedRad(P), getTopLevelPatchBasis(P));

    /* re-add self-emitted rad */
    colorAdd(getTopLevelPatchRad(P)[0], topLevelGalerkinElement(P)->sourceRad, getTopLevelPatchRad(P)[0]);

    /* clear un-shot and received radiance */
    stochasticRadiosityClearCoefficients(getTopLevelPatchUnShotRad(P), getTopLevelPatchBasis(P));
    stochasticRadiosityClearCoefficients(getTopLevelPatchReceivedRad(P), getTopLevelPatchBasis(P));
}

static void
randomWalkRadiosityDoShootingIteration(java::ArrayList<Patch *> *scenePatches) {
    long nr_walks;

    nr_walks = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays;
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
        nr_walks *= GLOBAL_stochasticRadiosity_approxDesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size;
    } else {
        nr_walks *= (long)std::pow(GLOBAL_stochasticRadiosity_approxDesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size, 1. / (1. -
                                                                                                                                                                  colorMaximumComponent(GLOBAL_statistics.averageReflectivity)));
    }

    fprintf(stderr, "Shooting iteration %d (%ld paths, approximately %ld rays)\n",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration,
            nr_walks, (long) std::floor((double) nr_walks / (1.0 - colorMaximumComponent(GLOBAL_statistics.averageReflectivity))));

    tracePaths(nr_walks,
               randomWalkRadiosityScalarSourcePower, randomWalkRadiosityScalarReflectance,
               randomWalkRadiosityShootingScore,
               randomWalkRadiosityShootingUpdate,
               scenePatches);
}

/**
Determines control radiosity value for collision gathering estimator
*/
static COLOR
randomWalkRadiosityDetermineGatheringControlRadiosity(java::ArrayList<Patch *> *scenePatches) {
    COLOR c1;
    COLOR c2;
    COLOR cr;

    colorClear(c1);
    colorClear(c2);

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        COLOR absorb;
        COLOR rho;
        COLOR Ed;
        COLOR num;
        COLOR denominator;
        Patch *patch = scenePatches->get(i);

        colorSetMonochrome(absorb, 1.0);
        rho = topLevelGalerkinElement(patch)->Rd;
        colorSubtract(absorb, rho, absorb); // 1-rho

        Ed = topLevelGalerkinElement(patch)->sourceRad;
        colorProduct(absorb, Ed, num);
        colorAddScaled(c1, patch->area, num, c1); // A_P (1-rho_P) E_P

        colorProduct(absorb, absorb, denominator);
        colorAddScaled(c2, patch->area, denominator, c2); // A_P (1-rho_P)^2
    }

    colorDivide(c1, c2, cr);
    fprintf(stderr, "Control radiosity value = ");
    cr.print(stderr);
    fprintf(stderr, ", luminosity = %g\n", colorLuminance(cr));

    return cr;
}

static void
randomWalkRadiosityCollisionGatheringScore(PATH *path, long /*nr_paths*/, double (* /*birthProb*/)(Patch *)) {
    COLOR accum_rad;
    int n;
    StochasticRaytracingPathNode *node = &path->nodes[path->numberOfNodes - 1];
    accum_rad = topLevelGalerkinElement(node->patch)->sourceRad;
    for ( n = path->numberOfNodes - 2, node--; n >= 0; n--, node-- ) {
        double uin = 0.0;
        double vin = 0.0;
        double uOut = 0.0;
        double vOut = 0.0;
        double r = 1.0;
        int i;
        Patch *P = node->patch;
        COLOR Rd = topLevelGalerkinElement(P)->Rd;
        colorProduct(Rd, accum_rad, accum_rad);

        P->uniformUv(&node->outpoint, &uOut, &vOut);
        if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
            r = 0.0;
            if ( n > 0 ) {
                // Not continuous random walk and not birth node
                P->uniformUv(&node->inPoint, &uin, &vin);
            }
        }

        for ( i = 0; i < getTopLevelPatchBasis(P)->size; i++ ) {
            double dual = getTopLevelPatchBasis(P)->dualFunction[i](uOut, vOut);    /* = dual basis f * area */
            colorAddScaled(getTopLevelPatchReceivedRad(P)[i], (float)dual, accum_rad, getTopLevelPatchReceivedRad(P)[i]);

            if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
                double basf = getTopLevelPatchBasis(P)->function[i](uin, vin);
                r += basf * dual;
            }
        }
        topLevelGalerkinElement(P)->ng++;

        colorScale((float)(r / node->probability), accum_rad, accum_rad);
        colorAdd(accum_rad, topLevelGalerkinElement(P)->sourceRad, accum_rad);
    }
}

static void
randomWalkRadiosityGatheringUpdate(Patch *P, double /*w*/) {
    // Use un-shot rad for accumulating sum of contributions
    stochasticRadiosityAddCoefficients(getTopLevelPatchUnShotRad(P), getTopLevelPatchReceivedRad(P),
                                       getTopLevelPatchBasis(P));
    stochasticRadiosityCopyCoefficients(getTopLevelPatchRad(P), getTopLevelPatchUnShotRad(P), getTopLevelPatchBasis(P));

    // Divide by nr of samples
    if ( topLevelGalerkinElement(P)->ng > 0 )
        stochasticRadiosityScaleCoefficients((1.0f / (float) topLevelGalerkinElement(P)->ng), getTopLevelPatchRad(P), getTopLevelPatchBasis(P));

    // Add source radiance (source term estimation suppression!)
    colorAdd(getTopLevelPatchRad(P)[0], topLevelGalerkinElement(P)->sourceRad, getTopLevelPatchRad(P)[0]);

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate ) {
        /* add constant control radiosity value */
        COLOR cr = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance;
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly ) {
            COLOR Rd = topLevelGalerkinElement(P)->Rd;
            colorProduct(Rd, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance, cr);
        }
        colorAdd(getTopLevelPatchRad(P)[0], cr, getTopLevelPatchRad(P)[0]);
    }

    stochasticRadiosityClearCoefficients(getTopLevelPatchReceivedRad(P), getTopLevelPatchBasis(P));
}

/**
Returns true when converged and false if not
*/
static void
randomWalkRadiosityDoGatheringIteration(java::ArrayList<Patch *> *scenePatches) {
    long nr_walks = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays;
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
        nr_walks *= GLOBAL_stochasticRadiosity_approxDesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size;
    } else {
        nr_walks *= (long)std::pow(GLOBAL_stochasticRadiosity_approxDesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size, 1. / (1. -
                                                                                                                                                                  colorMaximumComponent(GLOBAL_statistics.averageReflectivity)));
    }

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate && GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration == 1 ) {
        // Constant control variate for gathering random walk radiosity
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance = randomWalkRadiosityDetermineGatheringControlRadiosity(scenePatches);
        randomWalkRadiosityReduceSource(scenePatches); // Do this only once!
    }

    fprintf(stderr, "Collision gathering iteration %d (%ld paths, approximately %ld rays)\n",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration,
            nr_walks, (long) floor((double) nr_walks / (1.0 - colorMaximumComponent(GLOBAL_statistics.averageReflectivity))));

    tracePaths(nr_walks,
               randomWalkRadiosityPatchArea, randomWalkRadiosityScalarReflectance,
               randomWalkRadiosityCollisionGatheringScore,
               randomWalkRadiosityGatheringUpdate,
               scenePatches);
}

static void
randomWalkRadiosityUpdateSourceIllumination(StochasticRadiosityElement *elem, double /*w*/) {
    stochasticRadiosityCopyCoefficients(elem->radiance, elem->receivedRadiance, elem->basis);
    elem->sourceRad = elem->receivedRadiance[0];
    stochasticRadiosityClearCoefficients(elem->unShotRadiance, elem->basis);
    stochasticRadiosityClearCoefficients(elem->receivedRadiance, elem->basis);
}

static void
randomWalkRadiosityDoFirstShot(java::ArrayList<Patch *> *scenePatches) {
    long nr_rays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays * GLOBAL_stochasticRadiosity_approxDesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size;
    fprintf(stderr, "First shot (%ld rays):\n", nr_rays);
    doStochasticJacobiIteration(nr_rays, randomWalkRadiosityGetSelfEmittedRadiance, nullptr,
                                randomWalkRadiosityUpdateSourceIllumination, scenePatches);
    randomWalkRadiosityPrintStats();
}

void
RandomWalkRadianceMethod::terminate(java::ArrayList<Patch *> *scenePatches) {
    monteCarloRadiosityTerminate(scenePatches);
}

int
RandomWalkRadianceMethod::doStep(java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches) {
    monteCarloRadiosityPreStep(scenePatches);

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration == 1 ) {
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly ) {
            randomWalkRadiosityDoFirstShot(scenePatches);
        }
    }

    switch ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorType ) {
        case RW_SHOOTING:
            randomWalkRadiosityDoShootingIteration(scenePatches);
            break;
        case RW_GATHERING:
            randomWalkRadiosityDoGatheringIteration(scenePatches);
            break;
        default:
            logFatal(-1, "randomWalkRadiosityDoStep", "Unknown random walk estimator type %d",
                     GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorType);
    }

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        monteCarloRadiosityPatchComputeNewColor(scenePatches->get(i));
    }

    return false; // Never converged
}

#define STRING_SIZE 2000

char *
RandomWalkRadianceMethod::getStats() {
    static char stats[STRING_SIZE];
    char *p;
    int n;

    p = stats;
    snprintf(p, STRING_SIZE, "Random Walk Radiosity\nStatistics\n\n%n", &n);
    p += n;
    snprintf(p, STRING_SIZE, "Iteration nr: %d\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration, &n);
    p += n;
    snprintf(p, STRING_SIZE, "CPU time: %g secs\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, &n);
    p += n;
    snprintf(p, STRING_SIZE, "Radiance rays: %ld\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays, &n);
    p += n;
    snprintf(p, STRING_SIZE, "Importance rays: %ld\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays, &n);

    return stats;
}
