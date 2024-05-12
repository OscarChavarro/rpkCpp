#include <cstdio>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/Statistics.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/tracepath.h"
#include "raycasting/stochasticRaytracing/stochjacobi.h"
#include "raycasting/stochasticRaytracing/RandomWalkRadianceMethod.h"
#include "scene/Background.h"

#ifdef RAYTRACING_ENABLED

RandomWalkRadianceMethod::RandomWalkRadianceMethod() {
    monteCarloRadiosityDefaults();
    className = RANDOM_WALK;
}

RandomWalkRadianceMethod::~RandomWalkRadianceMethod() {
}

const char *
RandomWalkRadianceMethod::getRadianceMethodName() const {
    return "Random walk";
}

void
RandomWalkRadianceMethod::parseOptions(int *argc, char **argv) {
    randomWalkRadiosityParseOptions(argc, argv);
}

ColorRgb
RandomWalkRadianceMethod::getRadiance(
    Camera *camera,
    Patch *patch,
    double u,
    double v,
    Vector3D dir,
    const RenderOptions *renderOptions) const
{
    return monteCarloRadiosityGetRadiance(patch, u, v, dir, renderOptions);
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
RandomWalkRadianceMethod::renderScene(const Scene * /*scene*/, const RenderOptions * /*renderOptions*/) const {
}

void
RandomWalkRadianceMethod::writeVRML(const Camera * /*camera*/, FILE * /*fp*/, const RenderOptions * /*renderOptions*/) const {
}

void
RandomWalkRadianceMethod::initialize(Scene * /*scene*/) {
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
    ColorRgb radiance = topLevelStochasticRadiosityElement(P)->sourceRad;
    return /* M_PI * */ P->area * radiance.sumAbsComponents();
}

/**
Returns a double instead of a float in order to make it useful as
a survival stochasticJacobiProbability function
*/
static double
randomWalkRadiosityScalarReflectance(Patch *P) {
    return monteCarloRadiosityScalarReflectance(P);
}

static ColorRgb *
randomWalkRadiosityGetSelfEmittedRadiance(StochasticRadiosityElement *elem) {
    static ColorRgb Ed[MAX_BASIS_SIZE];
    stochasticRadiosityClearCoefficients(Ed, elem->basis);
    Ed[0] = topLevelStochasticRadiosityElement(elem->patch)->Ed; // Emittance
    return Ed;
}

/**
Subtracts (1 - rho) * control radiosity from the source radiosity of each patch
*/
static void
randomWalkRadiosityReduceSource(java::ArrayList<Patch *> *scenePatches) {
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        ColorRgb newSourceRadiance;
        ColorRgb rho;

        newSourceRadiance.setMonochrome(1.0);
        rho = topLevelStochasticRadiosityElement(patch)->Rd; // Reflectance
        newSourceRadiance.subtract(newSourceRadiance, rho); // 1 - rho
        newSourceRadiance.selfScalarProduct(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance); // (1-rho) * beta
        newSourceRadiance.subtract(topLevelStochasticRadiosityElement(patch)->sourceRad, newSourceRadiance); // E - (1-rho) * beta
        topLevelStochasticRadiosityElement(patch)->sourceRad = newSourceRadiance;
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
    ColorRgb accumPow;
    int n;
    StochasticRaytracingPathNode *node = &path->nodes[0];

    // path->nodes[0].probability is birth probability of the path
    accumPow.scaledCopy((float) (node->patch->area / node->probability), topLevelStochasticRadiosityElement(node->patch)->sourceRad);
    for ( n = 1, node++; n < path->numberOfNodes; n++, node++ ) {
        double uin = 0.0;
        double vin = 0.0;
        double uOut = 0.0;
        double vOut = 0.0;
        double r = 1.0;
        double w;
        int i;
        Patch *P = node->patch;
        ColorRgb Rd = topLevelStochasticRadiosityElement(P)->Rd;
        accumPow.scalarProduct(accumPow, Rd);

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
            getTopLevelPatchReceivedRad(P)[i].addScaled(
                getTopLevelPatchReceivedRad(P)[i],
                (float) (w * dual / (double) nr_paths),
                accumPow);

            if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
                double basf = getTopLevelPatchBasis(P)->function[i](uOut, vOut);
                r += dual * P->area * basf;
            }
        }

        accumPow.scale((float) (r / node->probability));
    }
}

static void
randomWalkRadiosityShootingUpdate(Patch *P, double w) {
    double k, old_quality;
    old_quality = topLevelStochasticRadiosityElement(P)->quality;
    topLevelStochasticRadiosityElement(P)->quality += (float)w;
    if ( topLevelStochasticRadiosityElement(P)->quality < EPSILON ) {
        return;
    }
    k = old_quality / topLevelStochasticRadiosityElement(P)->quality;

    // Subtract self-emitted rad
    getTopLevelPatchRad(P)[0].subtract(getTopLevelPatchRad(P)[0], topLevelStochasticRadiosityElement(P)->sourceRad);

    // Weight with previous results
    stochasticRadiosityScaleCoefficients((float)k, getTopLevelPatchRad(P), getTopLevelPatchBasis(P));
    stochasticRadiosityScaleCoefficients((1.0f - (float)k), getTopLevelPatchReceivedRad(P), getTopLevelPatchBasis(P));
    stochasticRadiosityAddCoefficients(getTopLevelPatchRad(P), getTopLevelPatchReceivedRad(P), getTopLevelPatchBasis(P));

    // Re-add self-emitted rad
    getTopLevelPatchRad(P)[0].add(
        getTopLevelPatchRad(P)[0], topLevelStochasticRadiosityElement(P)->sourceRad);

    // Clear un-shot and received radiance
    stochasticRadiosityClearCoefficients(getTopLevelPatchUnShotRad(P), getTopLevelPatchBasis(P));
    stochasticRadiosityClearCoefficients(getTopLevelPatchReceivedRad(P), getTopLevelPatchBasis(P));
}

static void
randomWalkRadiosityDoShootingIteration(
    VoxelGrid *sceneWorldVoxelGrid,
    java::ArrayList<Patch *> *scenePatches)
{
    long numberOfWalks;

    numberOfWalks = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays;
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
        numberOfWalks *= GLOBAL_stochasticRadiosity_approxDesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size;
    } else {
        numberOfWalks *= (long)java::Math::pow(GLOBAL_stochasticRadiosity_approxDesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size, 1. / (1. -
                                                                                                                                                                             GLOBAL_statistics.averageReflectivity.maximumComponent()));
    }

    fprintf(stderr, "Shooting iteration %d (%ld paths, approximately %ld rays)\n",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration,
            numberOfWalks, (long) std::floor((double) numberOfWalks / (1.0 -
                                                                       GLOBAL_statistics.averageReflectivity.maximumComponent())));

    tracePaths(
        sceneWorldVoxelGrid,
        numberOfWalks,
        randomWalkRadiosityScalarSourcePower, randomWalkRadiosityScalarReflectance,
        randomWalkRadiosityShootingScore,
        randomWalkRadiosityShootingUpdate,
        scenePatches);
}

/**
Determines control radiosity value for collision gathering estimator
*/
static ColorRgb
randomWalkRadiosityDetermineGatheringControlRadiosity(java::ArrayList<Patch *> *scenePatches) {
    ColorRgb c1;
    ColorRgb c2;
    ColorRgb cr;

    c1.clear();
    c2.clear();

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        ColorRgb absorb;
        ColorRgb rho;
        ColorRgb Ed;
        ColorRgb num;
        ColorRgb denominator;
        Patch *patch = scenePatches->get(i);

        absorb.setMonochrome(1.0);
        rho = topLevelStochasticRadiosityElement(patch)->Rd;
        absorb.subtract(absorb, rho); // 1-rho

        Ed = topLevelStochasticRadiosityElement(patch)->sourceRad;
        num.scalarProduct(absorb, Ed);
        c1.addScaled(c1, patch->area, num); // A_P (1-rho_P) E_P

        denominator.scalarProduct(absorb, absorb);
        c2.addScaled(c2, patch->area, denominator); // A_P (1-rho_P)^2
    }

    cr.divide(c1, c2);
    fprintf(stderr, "Control radiosity value = ");
    cr.print(stderr);
    fprintf(stderr, ", luminosity = %g\n", cr.luminance());

    return cr;
}

static void
randomWalkRadiosityCollisionGatheringScore(PATH *path, long /*nr_paths*/, double (* /*birthProb*/)(Patch *)) {
    ColorRgb accumRad;
    int n;
    StochasticRaytracingPathNode *node = &path->nodes[path->numberOfNodes - 1];
    accumRad = topLevelStochasticRadiosityElement(node->patch)->sourceRad;
    for ( n = path->numberOfNodes - 2, node--; n >= 0; n--, node-- ) {
        double uin = 0.0;
        double vin = 0.0;
        double uOut = 0.0;
        double vOut = 0.0;
        double r = 1.0;
        int i;
        Patch *P = node->patch;
        ColorRgb Rd = topLevelStochasticRadiosityElement(P)->Rd;
        accumRad.selfScalarProduct(Rd);

        P->uniformUv(&node->outpoint, &uOut, &vOut);
        if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
            r = 0.0;
            if ( n > 0 ) {
                // Not continuous random walk and not birth node
                P->uniformUv(&node->inPoint, &uin, &vin);
            }
        }

        for ( i = 0; i < getTopLevelPatchBasis(P)->size; i++ ) {
            double dual = getTopLevelPatchBasis(P)->dualFunction[i](uOut, vOut); // = dual basis f * area
            getTopLevelPatchReceivedRad(P)[i].addScaled(getTopLevelPatchReceivedRad(P)[i], (float) dual, accumRad);

            if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
                double basf = getTopLevelPatchBasis(P)->function[i](uin, vin);
                r += basf * dual;
            }
        }
        topLevelStochasticRadiosityElement(P)->ng++;

        accumRad.scale((float) (r / node->probability));
        accumRad.add(accumRad, topLevelStochasticRadiosityElement(P)->sourceRad);
    }
}

static void
randomWalkRadiosityGatheringUpdate(Patch *P, double /*w*/) {
    // Use un-shot rad for accumulating sum of contributions
    stochasticRadiosityAddCoefficients(getTopLevelPatchUnShotRad(P), getTopLevelPatchReceivedRad(P),
                                       getTopLevelPatchBasis(P));
    stochasticRadiosityCopyCoefficients(getTopLevelPatchRad(P), getTopLevelPatchUnShotRad(P), getTopLevelPatchBasis(P));

    // Divide by nr of samples
    if ( topLevelStochasticRadiosityElement(P)->ng > 0 )
        stochasticRadiosityScaleCoefficients((1.0f / (float) topLevelStochasticRadiosityElement(P)->ng), getTopLevelPatchRad(P), getTopLevelPatchBasis(P));

    // Add source radiance (source term estimation suppression!)
    getTopLevelPatchRad(P)[0].add(getTopLevelPatchRad(P)[0], topLevelStochasticRadiosityElement(P)->sourceRad);

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate ) {
        // Add constant control radiosity value
        ColorRgb cr = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance;
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly ) {
            ColorRgb Rd = topLevelStochasticRadiosityElement(P)->Rd;
            cr.scalarProduct(Rd, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance);
        }
        getTopLevelPatchRad(P)[0].add(getTopLevelPatchRad(P)[0], cr);
    }

    stochasticRadiosityClearCoefficients(getTopLevelPatchReceivedRad(P), getTopLevelPatchBasis(P));
}

/**
Returns true when converged and false if not
*/
static void
randomWalkRadiosityDoGatheringIteration(
    VoxelGrid *sceneWorldVoxelGrid,
    java::ArrayList<Patch *> *scenePatches)
{
    long numberOfWalks = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays;
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.continuousRandomWalk ) {
        numberOfWalks *= GLOBAL_stochasticRadiosity_approxDesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size;
    } else {
        numberOfWalks *= (long)java::Math::pow(
            GLOBAL_stochasticRadiosity_approxDesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size,
            1.0 / (1.0 - GLOBAL_statistics.averageReflectivity.maximumComponent()));
    }

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.constantControlVariate && GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration == 1 ) {
        // Constant control variate for gathering random walk radiosity
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.controlRadiance = randomWalkRadiosityDetermineGatheringControlRadiosity(scenePatches);
        randomWalkRadiosityReduceSource(scenePatches); // Do this only once!
    }

    fprintf(stderr, "Collision gathering iteration %d (%ld paths, approximately %ld rays)\n",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration,
            numberOfWalks, (long) floor((double) numberOfWalks / (1.0 - GLOBAL_statistics.averageReflectivity.maximumComponent())));

    tracePaths(
            sceneWorldVoxelGrid,
            numberOfWalks,
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
randomWalkRadiosityDoFirstShot(
    VoxelGrid *sceneWorldVoxelGrid,
    java::ArrayList<Patch *> *scenePatches,
    RenderOptions *renderOptions)
{
    long numberOfRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays *
        GLOBAL_stochasticRadiosity_approxDesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size;
    fprintf(stderr, "First shot (%ld rays):\n", numberOfRays);
    doStochasticJacobiIteration(sceneWorldVoxelGrid, numberOfRays, randomWalkRadiosityGetSelfEmittedRadiance, nullptr,
                                randomWalkRadiosityUpdateSourceIllumination, scenePatches, renderOptions);
    randomWalkRadiosityPrintStats();
}

void
RandomWalkRadianceMethod::terminate(java::ArrayList<Patch *> *scenePatches) {
    monteCarloRadiosityTerminate(scenePatches);
}

bool
RandomWalkRadianceMethod::doStep(Scene *scene, RenderOptions *renderOptions) {
    monteCarloRadiosityPreStep(scene, renderOptions);

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration == 1 ) {
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectOnly ) {
            randomWalkRadiosityDoFirstShot(scene->voxelGrid, scene->patchList, renderOptions);
        }
    }

    switch ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorType ) {
        case RW_SHOOTING:
            randomWalkRadiosityDoShootingIteration(scene->voxelGrid, scene->patchList);
            break;
        case RW_GATHERING:
            randomWalkRadiosityDoGatheringIteration(scene->voxelGrid, scene->patchList);
            break;
        default:
            logFatal(-1, "randomWalkRadiosityDoStep", "Unknown random walk estimator type %d",
                     GLOBAL_stochasticRaytracing_monteCarloRadiosityState.randomWalkEstimatorType);
    }

    for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
        monteCarloRadiosityPatchComputeNewColor(scene->patchList->get(i));
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

#endif
