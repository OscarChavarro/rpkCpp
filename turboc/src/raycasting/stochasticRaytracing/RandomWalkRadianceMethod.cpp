#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "java/util/Formatter.h"
#include "common/Error.h"
#include "common/RenderOptions.h"
#include "common/statistics/Statistics.h"
#include "raycasting/stochasticRaytracing/Basismcrad.h"
#include "raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "raycasting/stochasticRaytracing/McradP.h"
#include "raycasting/stochasticRaytracing/RandomWalkRadianceMethod.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"
#include "raycasting/stochasticRaytracing/Stochjacobi.h"
#include "raycasting/stochasticRaytracing/Tracepath.h"

#ifdef RAYTRACING_ENABLED

 void
RandomWalkRadianceMethod::appendRandomWalkStatsText(char *buffer, int *offset, const char *format, ...) {
    if ( *offset >= RANDOM_WALK_STRING_LENGTH - 1 ) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    const int available = RANDOM_WALK_STRING_LENGTH - *offset;
    const int written = Formatter::vformat(&buffer[*offset], available, format, arguments);
    va_end(arguments);

    if ( written <= 0 ) {
        return;
    }
    if ( written >= available ) {
        *offset = RANDOM_WALK_STRING_LENGTH - 1;
    } else {
        *offset += written;
    }
}

RandomWalkRadianceMethod::RandomWalkRadianceMethod(
    StochasticRelaxation &inStochasticRelaxationState,
    ElementHierarchyState &inElementHierarchyState,
    StochasticRadiosityBasisState &inStochasticRadiosityBasisState):
    stochasticRelaxationState(inStochasticRelaxationState),
    elementHierarchyState(inElementHierarchyState),
    stochasticRadiosityBasisState(inStochasticRadiosityBasisState)
{
    StochasticRelaxation::setActiveState(stochasticRelaxationState);
    ElementHierarchyState::setActiveState(elementHierarchyState);
    StochasticRadiosityBasisState::setActiveState(stochasticRadiosityBasisState);
    Mcrad::monteCarloRadiosityDefaults();
    className = RANDOM_WALK;
}

RandomWalkRadianceMethod::~RandomWalkRadianceMethod() {
}

const char *
RandomWalkRadianceMethod::getRadianceMethodName() const {
    return "Random walk";
}

void
RandomWalkRadianceMethod::parseOptions(int */*argc*/, char **/*argv*/) {
}

ColorRgb
RandomWalkRadianceMethod::getRadiance(
    Camera */*camera*/,
    Patch *patch,
    double u,
    double v,
    Vector3D dir,
    const RenderOptions *renderOptions) const
{
    StochasticRelaxation::setActiveState(((StochasticRelaxation &)(stochasticRelaxationState)));
    ElementHierarchyState::setActiveState(((ElementHierarchyState &)(elementHierarchyState)));
    StochasticRadiosityBasisState::setActiveState(((StochasticRadiosityBasisState &)(stochasticRadiosityBasisState)));
    return Mcrad::monteCarloRadiosityGetRadiance(patch, u, v, dir, renderOptions);
}

Element *
RandomWalkRadianceMethod::createPatchData(Patch *patch) {
    return Mcrad::mntCarloRadCreatePtchData(patch);
}

void
RandomWalkRadianceMethod::destroyPatchData(Patch *patch) {
    Mcrad::mntCarloRadDestroyPtchData(patch);
}

void
RandomWalkRadianceMethod::writeVRML(
    const Camera * /*camera*/,
    OutputStream * /*outputStream*/,
    const RenderOptions * /*renderOptions*/) const
{
}

void
RandomWalkRadianceMethod::initialize(Scene *scene, ToneMappingContext *toneMapOptions) {
    StochasticRelaxation::setActiveState(stochasticRelaxationState);
    ElementHierarchyState::setActiveState(elementHierarchyState);
    StochasticRadiosityBasisState::setActiveState(stochasticRadiosityBasisState);
    (void) scene;
    StochasticRelaxation::activeState().toneMapOptions = toneMapOptions;
    if ( StochasticRelaxation::activeState().toneMapOptions == NULL ) {
        Error::fatal(-1, "RandomWalkRadianceMethod::initialize", "Tone mapping context not provided");
    }
    StochasticRelaxation::activeState().method = RANDOM_WALK_RADIOSITY_METHOD;
    Mcrad::monteCarloRadiosityInit();
}

 void
RandomWalkRadianceMethod::randomWalkRadiosityPrintStats() {
    System::err.printf("%g secs., total radiance rays = %ld",
            StochasticRelaxation::activeState().cpuSeconds, StochasticRelaxation::activeState().tracedRays);
    System::err.printf(", total flux = ");
    StochasticRelaxation::activeState().totalFlux.print(&System::err);
    if ( StochasticRelaxation::activeState().importanceDriven ) {
        System::err.printf("\ntotal importance rays = %ld, total importance = %g, total area = %g",
                StochasticRelaxation::activeState().importanceTracedRays, StochasticRelaxation::activeState().totalYmp, Statistics::instance().radiance.totalArea);
    }
    System::err.printf("\n");
}

/**
Used as un-normalised stochasticJacobiProbability for mimicking global lines
*/
 double
RandomWalkRadianceMethod::randomWalkRadiosityPatchArea(const Patch *P) {
    return P->area;
}

/**
stochasticJacobiProbability proportional to power to be propagated
*/
 double
RandomWalkRadianceMethod::rndmWalkRadSclrSrcPwr(const Patch *P) {
    ColorRgb radiance = McradP::topLvlStochRadElem(P)->sourceRad;
    return P->area * radiance.sumAbsComponents();
}

/**
Returns a double instead of a float in order to make it useful as
a survival stochasticJacobiProbability function
*/
 double
RandomWalkRadianceMethod::rndmWalkRadSclrRefl(const Patch *P) {
    return Mcrad::mntCarloRadSclrRefl(P);
}

 ColorRgb *
RandomWalkRadianceMethod::rndmWalkRadGetSelfEmitRadn(const StochasticRadiosityElement *elem) {
    static ColorRgb Ed[GALERKIN_MAX_BASIS_SIZE];
    Coefficientsmcrad::stchsRadClearCoeff(Ed, elem->basis);
    Ed[0] = McradP::topLvlStochRadElem(elem->patch)->Ed; // Emittance
    return Ed;
}

/**
Subtracts (1 - rho) * control radiosity from the source radiosity of each patch
*/
 void
RandomWalkRadianceMethod::randomWalkRadiosityReduceSource(const ArrayList<Patch *> *scenePatches) {
    for ( int i = 0; scenePatches != NULL && i < scenePatches->size(); i++ ) {
        const Patch *patch = scenePatches->get(i);
        ColorRgb newSourceRadiance;
        ColorRgb rho;

        newSourceRadiance.setMonochrome(1.0);
        rho = McradP::topLvlStochRadElem(patch)->Rd; // Reflectance
        newSourceRadiance.subtract(newSourceRadiance, rho); // 1 - rho
        newSourceRadiance.selfScalarProduct(StochasticRelaxation::activeState().controlRadiance); // (1-rho) * beta
        newSourceRadiance.subtract(McradP::topLvlStochRadElem(patch)->sourceRad, newSourceRadiance); // E - (1-rho) * beta
        McradP::topLvlStochRadElem(patch)->sourceRad = newSourceRadiance;
    }
}

 double
RandomWalkRadianceMethod::randomWalkRadiosityScoreWeight(const Path *path, int n) {
    double w = 0.0;
    int t = path->numberOfNodes - ((StochasticRelaxation::activeState().randomWalkNumLast > 0) ? StochasticRelaxation::activeState().randomWalkNumLast : 1);

    switch ( StochasticRelaxation::activeState().randomWalkEstimatorKind ) {
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
                const int lastNodeIndex = path->numberOfNodes - 1;
                w = 1.0 / (1.0 - path->nodes[lastNodeIndex].probability);
                // Absorption prob of the last node
                for ( int nodeIndex = lastNodeIndex - 1; nodeIndex >= n; nodeIndex-- ) {
                    // Survival prob of n...numberOfNodes-2th node
                    w /= path->nodes[nodeIndex].probability;
                }
            }
            break;
        case RW_N_LAST:
            if ( n == t ) {
                // 1 / absorption probability of the last path node
                w = 1.0 / (1.0 - path->nodes[path->numberOfNodes - 1].probability);
            } else if ( n > t ) {
                w = 1.0;
            }
            break;
        default:
            Error::fatal(-1, "randomWalkRadiosityScoreWeight", "Unknown random walk estimator kind %d",
                     StochasticRelaxation::activeState().randomWalkEstimatorKind);
    }
    return w;
}

 void
RandomWalkRadianceMethod::rndmWalkRadShootScr(const Path *path, long nr_paths, double (* /*birthProb*/)(const Patch *)) {
    ColorRgb accumPow;
    const StochasticRaytracingPathNode &firstNode = path->nodes[0];

    // path->nodes[0].probability is birth probability of the path
    accumPow.scaledCopy(((float)(firstNode.patch->area / firstNode.probability)), McradP::topLvlStochRadElem(firstNode.patch)->sourceRad);
    for ( int n = 1; n < path->numberOfNodes; n++ ) {
        const StochasticRaytracingPathNode &node = path->nodes[n];
        double uin = 0.0;
        double vin = 0.0;
        double uOut = 0.0;
        double vOut = 0.0;
        double r = 1.0;
        double w;
        const Patch *P = node.patch;
        ColorRgb Rd = McradP::topLvlStochRadElem(P)->Rd;
        accumPow.scalarProduct(accumPow, Rd);

        P->uniformUv(&node.inPoint, &uin, &vin);
        if ( !StochasticRelaxation::activeState().continuousRandomWalk ) {
            r = 0.0;
            if ( n < path->numberOfNodes - 1 ) {
                // Not continuous random walk and not node of absorption
                P->uniformUv(&node.outpoint, &uOut, &vOut);
            }
        }

        w = randomWalkRadiosityScoreWeight(path, n);

        for ( int i = 0; i < McradP::getTopLevelPatchBasis(P)->size; i++ ) {
            double dual = McradP::getTopLevelPatchBasis(P)->dualFunction[i](uin, vin) / P->area;
            McradP::getTopLevelPatchReceivedRad(P)[i].addScaled(
                McradP::getTopLevelPatchReceivedRad(P)[i],
                ((float)(w * dual / ((double)(nr_paths)))),
                accumPow);

            if ( !StochasticRelaxation::activeState().continuousRandomWalk ) {
                double basf = McradP::getTopLevelPatchBasis(P)->function[i](uOut, vOut);
                r += dual * P->area * basf;
            }
        }

        accumPow.scale(((float)(r / node.probability)));
    }
}

 void
RandomWalkRadianceMethod::rndmWalkRadShootUpd(const Patch *P, double w) {
    double k;
    double oldQuality;
    oldQuality = McradP::topLvlStochRadElem(P)->quality;
    McradP::topLvlStochRadElem(P)->quality += ((float)(w));
    if ( McradP::topLvlStochRadElem(P)->quality < Numeric::EPSILON ) {
        return;
    }
    k = oldQuality / McradP::topLvlStochRadElem(P)->quality;

    // Subtract self-emitted rad
    McradP::getTopLevelPatchRad(P)[0].subtract(McradP::getTopLevelPatchRad(P)[0], McradP::topLvlStochRadElem(P)->sourceRad);

    // Weight with previous results
    Coefficientsmcrad::stchsRadScaleCoeff(((float)(k)), McradP::getTopLevelPatchRad(P), McradP::getTopLevelPatchBasis(P));
    Coefficientsmcrad::stchsRadScaleCoeff((1.0f - ((float)(k))), McradP::getTopLevelPatchReceivedRad(P), McradP::getTopLevelPatchBasis(P));
    Coefficientsmcrad::stchsRadAddCoeff(McradP::getTopLevelPatchRad(P), McradP::getTopLevelPatchReceivedRad(P), McradP::getTopLevelPatchBasis(P));

    // Re-add self-emitted rad
    McradP::getTopLevelPatchRad(P)[0].add(
        McradP::getTopLevelPatchRad(P)[0], McradP::topLvlStochRadElem(P)->sourceRad);

    // Clear un-shot and received radiance
    Coefficientsmcrad::stchsRadClearCoeff(McradP::getTopLevelPatchUnShotRad(P), McradP::getTopLevelPatchBasis(P));
    Coefficientsmcrad::stchsRadClearCoeff(McradP::getTopLevelPatchReceivedRad(P), McradP::getTopLevelPatchBasis(P));
}

 void
RandomWalkRadianceMethod::rndmWalkRadDShootItrtn(
    const VoxelGrid *sceneWorldVoxelGrid,
    const ArrayList<Patch *> *scenePatches)
{
    long numberOfWalks;

    numberOfWalks = StochasticRelaxation::activeState().initialNumberOfRays;
    if ( StochasticRelaxation::activeState().continuousRandomWalk ) {
        numberOfWalks *= StochasticRadiosityBasisState::activeState().approxDesc[StochasticRelaxation::activeState().approximationOrderType].basis_size;
    } else {
        numberOfWalks *= ((long)(Math::pow(
            StochasticRadiosityBasisState::activeState().approxDesc[StochasticRelaxation::activeState().approximationOrderType].
            basis_size, 1. / (1. -
                              Statistics::instance().radiance.averageReflectivity.maximumComponent()))));
    }

    System::err.printf("Shooting iteration %d (%ld paths, approximately %ld rays)\n",
            StochasticRelaxation::activeState().currentIteration,
            numberOfWalks, ((long)(Math::floor(((double)(numberOfWalks)) /
                                                               (1.0 - Statistics::instance().radiance.averageReflectivity.maximumComponent())))));

    Tracepath::tracePaths(
        sceneWorldVoxelGrid,
        numberOfWalks,
        rndmWalkRadSclrSrcPwr,
        rndmWalkRadSclrRefl,
        rndmWalkRadShootScr,
        rndmWalkRadShootUpd,
        scenePatches);
}

/**
Determines control radiosity value for collision gathering estimator
*/
 ColorRgb
RandomWalkRadianceMethod::rndmWalkRadDetGthrnCtrlRad(const ArrayList<Patch *> *scenePatches) {
    ColorRgb c1;
    ColorRgb c2;
    ColorRgb cr;

    c1.clear();
    c2.clear();

    for ( int i = 0; scenePatches != NULL && i < scenePatches->size(); i++ ) {
        ColorRgb absorb;
        ColorRgb rho;
        ColorRgb Ed;
        ColorRgb num;
        ColorRgb denominator;
        const Patch *patch = scenePatches->get(i);

        absorb.setMonochrome(1.0);
        rho = McradP::topLvlStochRadElem(patch)->Rd;
        absorb.subtract(absorb, rho); // 1-rho

        Ed = McradP::topLvlStochRadElem(patch)->sourceRad;
        num.scalarProduct(absorb, Ed);
        c1.addScaled(c1, patch->area, num); // A_P (1-rho_P) E_P

        denominator.scalarProduct(absorb, absorb);
        c2.addScaled(c2, patch->area, denominator); // A_P (1-rho_P)^2
    }

    cr.divide(c1, c2);
    System::err.printf("Control radiosity value = ");
    cr.print(&System::err);
    System::err.printf(", luminosity = %g\n", cr.luminance());

    return cr;
}

 void
RandomWalkRadianceMethod::rndmWalkRadCllsnGthrnScr(const Path *path, long /*nr_paths*/, double (* /*birthProb*/)(const Patch *)) {
    ColorRgb accumRad;
    const int lastNodeIndex = path->numberOfNodes - 1;
    accumRad = McradP::topLvlStochRadElem(path->nodes[lastNodeIndex].patch)->sourceRad;
    for ( int n = lastNodeIndex - 1; n >= 0; n-- ) {
        const StochasticRaytracingPathNode &node = path->nodes[n];
        double uin = 0.0;
        double vin = 0.0;
        double uOut = 0.0;
        double vOut = 0.0;
        double r = 1.0;
        const Patch *P = node.patch;
        ColorRgb Rd = McradP::topLvlStochRadElem(P)->Rd;
        accumRad.selfScalarProduct(Rd);

        P->uniformUv(&node.outpoint, &uOut, &vOut);
        if ( !StochasticRelaxation::activeState().continuousRandomWalk ) {
            r = 0.0;
            if ( n > 0 ) {
                // Not continuous random walk and not birth node
                P->uniformUv(&node.inPoint, &uin, &vin);
            }
        }

        for ( int i = 0; i < McradP::getTopLevelPatchBasis(P)->size; i++ ) {
            double dual = McradP::getTopLevelPatchBasis(P)->dualFunction[i](uOut, vOut); // = dual basis f * area
            McradP::getTopLevelPatchReceivedRad(P)[i].addScaled(McradP::getTopLevelPatchReceivedRad(P)[i], ((float)(dual)), accumRad);

            if ( !StochasticRelaxation::activeState().continuousRandomWalk ) {
                double basf = McradP::getTopLevelPatchBasis(P)->function[i](uin, vin);
                r += basf * dual;
            }
        }
        McradP::topLvlStochRadElem(P)->ng++;

        accumRad.scale(((float)(r / node.probability)));
        accumRad.add(accumRad, McradP::topLvlStochRadElem(P)->sourceRad);
    }
}

 void
RandomWalkRadianceMethod::rndmWalkRadGthrnUpd(const Patch *P, double /*w*/) {
    // Use un-shot rad for accumulating sum of contributions
    Coefficientsmcrad::stchsRadAddCoeff(McradP::getTopLevelPatchUnShotRad(P), McradP::getTopLevelPatchReceivedRad(P),
                                       McradP::getTopLevelPatchBasis(P));
    Coefficientsmcrad::stchsRadCopyCoeff(McradP::getTopLevelPatchRad(P), McradP::getTopLevelPatchUnShotRad(P), McradP::getTopLevelPatchBasis(P));

    // Divide by nr of samples
    if ( McradP::topLvlStochRadElem(P)->ng > 0 )
        Coefficientsmcrad::stchsRadScaleCoeff((1.0f / McradP::topLvlStochRadElem(P)->ng), McradP::getTopLevelPatchRad(P), McradP::getTopLevelPatchBasis(P));

    // Add source radiance (source term estimation suppression!)
    McradP::getTopLevelPatchRad(P)[0].add(McradP::getTopLevelPatchRad(P)[0], McradP::topLvlStochRadElem(P)->sourceRad);

    if ( StochasticRelaxation::activeState().constantControlVariate ) {
        // Add constant control radiosity value
        ColorRgb cr = StochasticRelaxation::activeState().controlRadiance;
        if ( StochasticRelaxation::activeState().indirectOnly ) {
            ColorRgb Rd = McradP::topLvlStochRadElem(P)->Rd;
            cr.scalarProduct(Rd, StochasticRelaxation::activeState().controlRadiance);
        }
        McradP::getTopLevelPatchRad(P)[0].add(McradP::getTopLevelPatchRad(P)[0], cr);
    }

    Coefficientsmcrad::stchsRadClearCoeff(McradP::getTopLevelPatchReceivedRad(P), McradP::getTopLevelPatchBasis(P));
}

/**
Returns true when converged and false if not
*/
 void
RandomWalkRadianceMethod::rndmWalkRadDGthrnItrtn(
    const VoxelGrid *sceneWorldVoxelGrid,
    const ArrayList<Patch *> *scenePatches)
{
    long numberOfWalks = StochasticRelaxation::activeState().initialNumberOfRays;
    if ( StochasticRelaxation::activeState().continuousRandomWalk ) {
        numberOfWalks *= StochasticRadiosityBasisState::activeState().approxDesc[StochasticRelaxation::activeState().approximationOrderType].basis_size;
    } else {
        numberOfWalks *= ((long)(Math::pow(
            StochasticRadiosityBasisState::activeState().approxDesc[StochasticRelaxation::activeState().approximationOrderType].
            basis_size,
            1.0 / (1.0 - Statistics::instance().radiance.averageReflectivity.maximumComponent()))));
    }

    if ( StochasticRelaxation::activeState().constantControlVariate && StochasticRelaxation::activeState().currentIteration == 1 ) {
        // Constant control variate for gathering random walk radiosity
        StochasticRelaxation::activeState().controlRadiance = rndmWalkRadDetGthrnCtrlRad(scenePatches);
        randomWalkRadiosityReduceSource(scenePatches); // Do this only once!
    }

    System::err.printf("Collision gathering iteration %d (%ld paths, approximately %ld rays)\n",
        StochasticRelaxation::activeState().currentIteration,
        numberOfWalks, ((long)(Math::floor(((double)(numberOfWalks)) / (1.0 - Statistics::instance().radiance.averageReflectivity.maximumComponent())))));

    Tracepath::tracePaths(
        sceneWorldVoxelGrid,
        numberOfWalks,
        randomWalkRadiosityPatchArea,
        rndmWalkRadSclrRefl,
        rndmWalkRadCllsnGthrnScr,
        rndmWalkRadGthrnUpd,
        scenePatches);
}

 void
RandomWalkRadianceMethod::rndmWalkRadUpdSrcIllum(StochasticRadiosityElement *elem, double /*w*/) {
    Coefficientsmcrad::stchsRadCopyCoeff(elem->radiance, elem->receivedRadiance, elem->basis);
    elem->sourceRad = elem->receivedRadiance[0];
    Coefficientsmcrad::stchsRadClearCoeff(elem->unShotRadiance, elem->basis);
    Coefficientsmcrad::stchsRadClearCoeff(elem->receivedRadiance, elem->basis);
}

 void
RandomWalkRadianceMethod::randomWalkRadiosityDoFirstShot(
    VoxelGrid *sceneWorldVoxelGrid,
    const ArrayList<Patch *> *scenePatches,
    RenderOptions *renderOptions)
{
    long numberOfRays = StochasticRelaxation::activeState().initialNumberOfRays *
        StochasticRadiosityBasisState::activeState().approxDesc[StochasticRelaxation::activeState().approximationOrderType].basis_size;
    System::err.printf("First shot (%ld rays):\n", numberOfRays);
    StochasticJacobi::doStochasticJacobiIteration(sceneWorldVoxelGrid, numberOfRays, rndmWalkRadGetSelfEmitRadn, NULL,
                                rndmWalkRadUpdSrcIllum, scenePatches, renderOptions);
    randomWalkRadiosityPrintStats();
}

void
RandomWalkRadianceMethod::terminate(ArrayList<Patch *> *scenePatches) {
    StochasticRelaxation::setActiveState(stochasticRelaxationState);
    ElementHierarchyState::setActiveState(elementHierarchyState);
    StochasticRadiosityBasisState::setActiveState(stochasticRadiosityBasisState);
    Mcrad::monteCarloRadiosityTerminate(scenePatches);
}

bool
RandomWalkRadianceMethod::doStep(Scene *scene, RenderOptions *renderOptions) {
    StochasticRelaxation::setActiveState(stochasticRelaxationState);
    ElementHierarchyState::setActiveState(elementHierarchyState);
    StochasticRadiosityBasisState::setActiveState(stochasticRadiosityBasisState);
    Mcrad::monteCarloRadiosityPreStep(scene, renderOptions);

    if ( StochasticRelaxation::activeState().currentIteration == 1
        && StochasticRelaxation::activeState().indirectOnly ) {
        randomWalkRadiosityDoFirstShot(scene->voxelGrid, scene->patchList, renderOptions);
    }

    switch ( StochasticRelaxation::activeState().randomWalkEstimatorType ) {
        case RW_SHOOTING:
            rndmWalkRadDShootItrtn(scene->voxelGrid, scene->patchList);
            break;
        case RW_GATHERING:
            rndmWalkRadDGthrnItrtn(scene->voxelGrid, scene->patchList);
            break;
        default:
            Error::fatal(-1, "randomWalkRadiosityDoStep", "Unknown random walk estimator type %d",
                     StochasticRelaxation::activeState().randomWalkEstimatorType);
    }

    for ( int i = 0; scene->patchList != NULL && i < scene->patchList->size(); i++ ) {
        Mcrad::mntCarloRadPtchCompNewClr(scene->patchList->get(i));
    }

    return false; // Never converged
}

char *
RandomWalkRadianceMethod::getStats() const {
    StochasticRelaxation::setActiveState(((StochasticRelaxation &)(stochasticRelaxationState)));
    ElementHierarchyState::setActiveState(((ElementHierarchyState &)(elementHierarchyState)));
    StochasticRadiosityBasisState::setActiveState(((StochasticRadiosityBasisState &)(stochasticRadiosityBasisState)));
    static char stats[RANDOM_WALK_STRING_LENGTH];
    int statsOffset = 0;

    appendRandomWalkStatsText(stats, &statsOffset, "Random Walk Radiosity\nStatistics\n\n");
    appendRandomWalkStatsText(stats, &statsOffset, "Iteration nr: %d\n",
                              StochasticRelaxation::activeState().currentIteration);
    appendRandomWalkStatsText(stats, &statsOffset, "CPU time: %g secs\n",
                              StochasticRelaxation::activeState().cpuSeconds);
    appendRandomWalkStatsText(stats, &statsOffset, "Radiance rays: %ld\n",
                              StochasticRelaxation::activeState().tracedRays);
    appendRandomWalkStatsText(stats, &statsOffset, "Importance rays: %ld\n",
                              StochasticRelaxation::activeState().importanceTracedRays);

    return stats;
}

#endif
