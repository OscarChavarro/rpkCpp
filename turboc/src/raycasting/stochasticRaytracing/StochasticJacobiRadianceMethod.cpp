#include <stdlib.h>

#include "java/util/Formatter.h"
#include "common/RenderOptions.h"

/**
Stochastic Relaxation Radiosity (currently only stochastic Jacobi)
*/

#include "java/util/ArrayList.txx"
#include "java/lang/System.h"
#include "common/logging/Logger.h"
#include "common/statistics/Statistics.h"
#include "raycasting/stochasticRaytracing/Basismcrad.h"
#include "raycasting/stochasticRaytracing/McradP.h"
#include "raycasting/stochasticRaytracing/Hierarchy.h"
#include "raycasting/stochasticRaytracing/Stochjacobi.h"
#include "raycasting/stochasticRaytracing/StochasticJacobiRadianceMethod.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

void
StochasticJacobiRadianceMethod::appendStochasticStatsText(char *buffer, int *offset, const char *format, ...) {
    if ( *offset >= STOCHASTIC_JACOBI_STRING_LENGTH - 1 ) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    const int available = STOCHASTIC_JACOBI_STRING_LENGTH - *offset;
    const int written = Formatter::vformat(&buffer[*offset], available, format, arguments);
    va_end(arguments);

    if ( written <= 0 ) {
        return;
    }
    if ( written >= available ) {
        *offset = STOCHASTIC_JACOBI_STRING_LENGTH - 1;
    } else {
        *offset += written;
    }
}

#ifdef RAYTRACING_ENABLED
StochasticJacobiRadianceMethod::StochasticJacobiRadianceMethod(
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
    className = STOCHASTIC_JACOBI;
}

StochasticJacobiRadianceMethod::~StochasticJacobiRadianceMethod() {
}

const char *
StochasticJacobiRadianceMethod::getRadianceMethodName() const {
    return "Stochastic Jacobi";
}

void
StochasticJacobiRadianceMethod::parseOptions(int */*argc*/, char **/*argv*/) {
}

void
StochasticJacobiRadianceMethod::terminate(ArrayList<Patch *> *scenePatches) {
    StochasticRelaxation::setActiveState(stochasticRelaxationState);
    ElementHierarchyState::setActiveState(elementHierarchyState);
    StochasticRadiosityBasisState::setActiveState(stochasticRadiosityBasisState);
    Mcrad::monteCarloRadiosityTerminate(scenePatches);
}

ColorRgb
StochasticJacobiRadianceMethod::getRadiance(Camera */*camera*/, Patch *patch, double u, double v, Vector3D dir, const RenderOptions *renderOptions) const {
    StochasticRelaxation::setActiveState(((StochasticRelaxation &)(stochasticRelaxationState)));
    ElementHierarchyState::setActiveState(((ElementHierarchyState &)(elementHierarchyState)));
    StochasticRadiosityBasisState::setActiveState(((StochasticRadiosityBasisState &)(stochasticRadiosityBasisState)));
    return Mcrad::monteCarloRadiosityGetRadiance(patch, u, v, dir, renderOptions);
}

Element *
StochasticJacobiRadianceMethod::createPatchData(Patch *patch) {
    return Mcrad::mntCarloRadCreatePtchData(patch);
}

void
StochasticJacobiRadianceMethod::destroyPatchData(Patch *patch) {
    Mcrad::mntCarloRadDestroyPtchData(patch);
}

void
StochasticJacobiRadianceMethod::writeVRML(
    const Camera * /*camera*/,
    OutputStream * /*outputStream*/,
    const RenderOptions * /*renderOptions*/) const
{
}

void
StochasticJacobiRadianceMethod::initialize(Scene *scene, ToneMappingContext *toneMapOptions) {
    StochasticRelaxation::setActiveState(stochasticRelaxationState);
    ElementHierarchyState::setActiveState(elementHierarchyState);
    StochasticRadiosityBasisState::setActiveState(stochasticRadiosityBasisState);
    (void) scene;
    StochasticRelaxation::activeState().toneMapOptions = toneMapOptions;
    if ( StochasticRelaxation::activeState().toneMapOptions == NULL ) {
        Logger::fatal(-1, "StochasticJacobiRadianceMethod::initialize", "Tone mapping context not provided");
    }
    StochasticRelaxation::activeState().method = STCHS_RLXTN_RDSTY_MTHD;
    Mcrad::monteCarloRadiosityInit();
}

char *
StochasticJacobiRadianceMethod::getStats() const {
    StochasticRelaxation::setActiveState(((StochasticRelaxation &)(stochasticRelaxationState)));
    ElementHierarchyState::setActiveState(((ElementHierarchyState &)(elementHierarchyState)));
    StochasticRadiosityBasisState::setActiveState(((StochasticRadiosityBasisState &)(stochasticRadiosityBasisState)));
    static char stats[STOCHASTIC_JACOBI_STRING_LENGTH];
    int statsOffset = 0;

    appendStochasticStatsText(stats, &statsOffset, "Stochastic Relaxation Radiosity\nStatistics\n\n");
    appendStochasticStatsText(stats, &statsOffset, "Iteration nr: %d\n", StochasticRelaxation::activeState().currentIteration);
    appendStochasticStatsText(stats, &statsOffset, "CPU time: %g secs\n", StochasticRelaxation::activeState().cpuSeconds);
    appendStochasticStatsText(stats, &statsOffset, "%ld elements (%ld clusters, %ld surfaces)\n",
                              ElementHierarchyState::activeState().nr_elements, ElementHierarchyState::activeState().nr_clusters,
                              ElementHierarchyState::activeState().nr_elements - ElementHierarchyState::activeState().nr_clusters);
    appendStochasticStatsText(stats, &statsOffset, "Radiance rays: %ld\n", StochasticRelaxation::activeState().tracedRays);
    appendStochasticStatsText(stats, &statsOffset, "Importance rays: %ld\n", StochasticRelaxation::activeState().importanceTracedRays);

    return stats;
}

/**
Randomly returns floor(x) or ceil(x) so that the expected value is equal to x
*/
long
StochasticJacobiRadianceMethod::stchsRelaxRadRndRnd(float x) {
    long l = ((long)(Math::floor(x)));
    if ( drand48() < (x - ((float)(l))) ) {
        l++;
    }
    return l;
}

void
StochasticJacobiRadianceMethod::stchsRelaxRadRecompDispClrs(const ArrayList<Patch *> *scenePatches) {
    StochasticRadiosityElement *topElement = ElementHierarchyState::activeState().topCluster;
    if ( topElement != NULL ) {
        topElement->traverseClusterLeafElements(StochasticRadiosityElement::stchsRadElemCompNewVtxClrs);
        topElement->traverseClusterLeafElements(StochasticRadiosityElement::stchsRadElemAdjTVtxClrs);
    } else {
        for ( int i = 0; scenePatches != NULL && i < scenePatches->size(); i++ ) {
            Mcrad::mntCarloRadPtchCompNewClr(scenePatches->get(i));
        }
    }
}

/**
Computes quality factor on given leaf element (see PhD Phillipe Bekaert p.152).
In the basic algorithms by Neumann et al. the quality factor would
correspond to the inverse of the elementary ray power. The quality factor
indicates the quality of the radiosity solution on a given leaf element.
The quality factor after different iterations is additive. It is used in order
to properly merge the result of new iterations with the result of previous
iterations properly taking into account the number of rays and importance
distribution
*/
double
StochasticJacobiRadianceMethod::stchsRelaxRadQualFactor(const StochasticRadiosityElement *elem, double w) {
    if ( StochasticRelaxation::activeState().importanceDriven ) {
        return w * elem->importance;
    }
    return w / StochasticRadiosityElement::stchsRadElemSclrRefl(elem);
}

ColorRgb *
StochasticJacobiRadianceMethod::stchsRelaxRadElemUnShotRadn(const StochasticRadiosityElement *elem) {
    return elem->unShotRadiance;
}

void
StochasticJacobiRadianceMethod::stchsRelaxRadElemIncrRadn(StochasticRadiosityElement *elem, double w) {
    // Each incremental iteration computes a different contribution to the
    // solution. The quality factor of the result remains constant
    if ( StochasticRelaxation::activeState().discardIncremental ) {
        elem->quality = 0.0;
        static bool repeated = false;
        if ( !repeated ) {
            Logger::warning("stchsRelaxRadElemIncrRadn",
                       "Solution of incremental Jacobi steps receives zero quality");
        }
        repeated = true;
    } else {
        elem->quality = ((float)(stchsRelaxRadQualFactor(elem, w)));
    }

    Coefficientsmcrad::stchsRadAddCoeff(elem->radiance, elem->receivedRadiance, elem->basis);
    Coefficientsmcrad::stchsRadCopyCoeff(elem->unShotRadiance, elem->receivedRadiance, elem->basis);
    if ( StochasticRelaxation::activeState().setSource ) {
        // Copy direct illumination and forget self emitted illumination
        elem->radiance[0] = elem->sourceRad = elem->receivedRadiance[0];
    }
    Coefficientsmcrad::stchsRadClearCoeff(elem->receivedRadiance, elem->basis);
}

void
StochasticJacobiRadianceMethod::stchRelaRadPrinIncrRadnStat() {
    System::err.printf("%g secs., radiance rays = %ld (%ld not to background), un-shot flux = ",
            StochasticRelaxation::activeState().cpuSeconds, StochasticRelaxation::activeState().tracedRays, StochasticRelaxation::activeState().tracedRays - StochasticRelaxation::activeState().numberOfMisses);
    StochasticRelaxation::activeState().unShotFlux.print(&System::err);
    System::err.printf(", total flux = ");
    StochasticRelaxation::activeState().totalFlux.print(&System::err);
    System::err.printf(", indirect importance weighted un-shot flux = ");
    StochasticRelaxation::activeState().indrcImpWghtdUnShotFlux.print(&System::err);
    System::err.printf("\n");
}

void
StochasticJacobiRadianceMethod::stchsRelaxRadDIncrmRadnItrtn(
    Scene* scene,
    const RadianceMethod */*radianceMethod*/,
    RenderOptions *renderOptions)
{
    double refUnShot;
    long stepNumber = 0;

    int weightedSampling = StochasticRelaxation::activeState().weightedSampling;
    int importanceDriven = StochasticRelaxation::activeState().importanceDriven;
    if ( !StochasticRelaxation::activeState().incrementalUsesImportance ) {
        // Temporarily switch it off
        StochasticRelaxation::activeState().importanceDriven = false;
    }
    StochasticRelaxation::activeState().weightedSampling = false;

    stchRelaRadPrinIncrRadnStat();
    refUnShot = StochasticRelaxation::activeState().unShotFlux.sumAbsComponents();
    if ( StochasticRelaxation::activeState().incrementalUsesImportance ) {
        refUnShot = StochasticRelaxation::activeState().indrcImpWghtdUnShotFlux.sumAbsComponents();
    }
    while ( true ) {
        // Choose nr of rays so that power carried by each ray remains equal, and
        // proportional to the number of basis functions in the rad. approx
        double unShotFraction;
        long numberOfRays;
        unShotFraction = StochasticRelaxation::activeState().unShotFlux.sumAbsComponents() / refUnShot;
        if ( StochasticRelaxation::activeState().incrementalUsesImportance ) {
            unShotFraction = StochasticRelaxation::activeState().indrcImpWghtdUnShotFlux.sumAbsComponents() / refUnShot;
        }
        if ( unShotFraction < 0.01 ) {
            // Only 1/100th of self-emitted power remains un-shot
            break;
        }
        numberOfRays = stchsRelaxRadRndRnd(
                ((float)(unShotFraction * ((double)(StochasticRelaxation::activeState().initialNumberOfRays)) *
                                   StochasticRadiosityBasisState::activeState().approxDesc[StochasticRelaxation::activeState().
                                       approximationOrderType].basis_size)));

        stepNumber++;
        System::err.printf("Incremental radiance propagation step %ld: %.3f%% un-shot power left.\n",
                stepNumber, 100. * unShotFraction);

        StochasticJacobi::doStochasticJacobiIteration(
            scene->voxelGrid,
            numberOfRays,
            stchsRelaxRadElemUnShotRadn,
            NULL,
            stchsRelaxRadElemIncrRadn,
            scene->patchList,
            renderOptions);

        StochasticRelaxation::activeState().setSource = false; // Direct illumination is copied to SOURCE_FLUX(P) only the first time

        Mcrad::mntCarloRadUpdCpuSecs();
        stchRelaRadPrinIncrRadnStat();
        if ( unShotFraction > 0.3 ) {
            stchsRelaxRadRecompDispClrs(scene->patchList);
        }
    }

    StochasticRelaxation::activeState().importanceDriven = importanceDriven; // Switch it back on if it was on
    StochasticRelaxation::activeState().weightedSampling = weightedSampling;
}

float
StochasticJacobiRadianceMethod::stchsRelaxRadElemUnShotImp(const StochasticRadiosityElement *elem) {
    return elem->unShotImportance;
}

void
StochasticJacobiRadianceMethod::stchsRelaxRadElemIncrImp(StochasticRadiosityElement *elem, double /*w*/) {
    elem->importance += elem->receivedImportance;
    elem->unShotImportance = elem->receivedImportance;
    elem->receivedImportance = 0.0;
}

void
StochasticJacobiRadianceMethod::stchsRelaxRadPrintIncrmImpStats() {
    System::err.printf("%g secs., importance rays = %ld, un-shot importance = %g, total importance = %g, total area = %g\n",
            StochasticRelaxation::activeState().cpuSeconds, StochasticRelaxation::activeState().importanceTracedRays, StochasticRelaxation::activeState().unShotYmp, StochasticRelaxation::activeState().totalYmp, Statistics::instance().radiance.totalArea);
}

void
StochasticJacobiRadianceMethod::stchsRelaxRadDIncrmImpItrtn(
    VoxelGrid *sceneWorldVoxelGrid,
    const ArrayList<Patch *> *scenePatches,
    RenderOptions *renderOptions)
{
    long stepNumber = 0;
    int radiance_driven = StochasticRelaxation::activeState().radianceDriven;
    int do_h_meshing = ElementHierarchyState::activeState().do_h_meshing;
    HierarchyClusteringMode clustering = ElementHierarchyState::activeState().clustering;
    int weighted_sampling = StochasticRelaxation::activeState().weightedSampling;

    if ( StochasticRelaxation::activeState().sourceYmp < Numeric::EPSILON ) {
        System::err.printf("No source importance!!\n");
        return;
    }

    StochasticRelaxation::activeState().radianceDriven = false; // Temporary switch it off
    ElementHierarchyState::activeState().do_h_meshing = false;
    ElementHierarchyState::activeState().clustering = NO_CLUSTERING;
    StochasticRelaxation::activeState().weightedSampling = false;

    stchRelaRadPrinIncrRadnStat();
    while ( true ) {
        // Choose nr of rays so that power carried by each ray is the same, and
        // proportional to the number of basis functions in the rad. approx. */
        double unShotFraction = StochasticRelaxation::activeState().unShotYmp / StochasticRelaxation::activeState().sourceYmp;
        long numberOfRays = stchsRelaxRadRndRnd(
                ((float)(unShotFraction)) * ((float)(StochasticRelaxation::activeState().initialNumberOfRays)));
        if ( unShotFraction < 0.01 ) {
            break;
        }

        stepNumber++;
        System::err.printf("Incremental importance propagation step %ld: %.3f%% un-shot importance left.\n",
                stepNumber, 100.0 * unShotFraction);

        StochasticJacobi::doStochasticJacobiIteration(
            sceneWorldVoxelGrid,
            numberOfRays,
            NULL,
            stchsRelaxRadElemUnShotImp,
            stchsRelaxRadElemIncrImp,
            scenePatches,
            renderOptions);

        Mcrad::mntCarloRadUpdCpuSecs();
        stchsRelaxRadPrintIncrmImpStats();
    }

    StochasticRelaxation::activeState().radianceDriven = radiance_driven; // Switch on again
    ElementHierarchyState::activeState().do_h_meshing = do_h_meshing;
    ElementHierarchyState::activeState().clustering = clustering;
    StochasticRelaxation::activeState().weightedSampling = weighted_sampling;
}

ColorRgb *
StochasticJacobiRadianceMethod::stchsRelaxRadElemRadn(const StochasticRadiosityElement *elem) {
    return elem->radiance;
}

void
StochasticJacobiRadianceMethod::stchsRelaxRadElemUpdRadn(StochasticRadiosityElement *elem, double w) {
    double k = ((double)(StochasticRelaxation::activeState().prevTracedRays)) / ((double)(StochasticRelaxation::activeState().tracedRays > 0
                   ? StochasticRelaxation::activeState().tracedRays
                   : 1));

    if ( !StochasticRelaxation::activeState().naiveMerging ) {
        double quality = stchsRelaxRadQualFactor(elem, w);
        if ( elem->quality < Numeric::EPSILON ) {
            // Solution of this iteration takes over
            k = 0.0;
        } else if ( quality < Numeric::EPSILON ) {
            // Keep result of previous iterations
            k = 1.0;
        } else if ( elem->quality + quality > Numeric::EPSILON ) {
            k = elem->quality / (elem->quality + quality);
        } else {
            // Quality of new solution is so high that it must take over
            k = 0.0;
        }
        elem->quality += ((float)(quality)); // Add quality
    }

    // Subtract source radiosity
    elem->radiance[0].subtract(elem->radiance[0], elem->sourceRad);

    // Combine with previous results
    Coefficientsmcrad::stchsRadScaleCoeff(((float)(k)), elem->radiance, elem->basis);
    Coefficientsmcrad::stchsRadScaleCoeff((1.0f - ((float)(k))), elem->receivedRadiance, elem->basis);
    Coefficientsmcrad::stchsRadAddCoeff(elem->radiance, elem->receivedRadiance, elem->basis);

    // Re-add source radiosity
    elem->radiance[0].add(elem->radiance[0], elem->sourceRad);

    // Clear un-shot and received radiance
    Coefficientsmcrad::stchsRadClearCoeff(elem->unShotRadiance, elem->basis);
    Coefficientsmcrad::stchsRadClearCoeff(elem->receivedRadiance, elem->basis);
}

void
StochasticJacobiRadianceMethod::stchsRelaxRadPrintRegStats() {
    System::err.printf("%g secs., radiance rays = %ld (%ld not to background), un-shot flux = ",
            StochasticRelaxation::activeState().cpuSeconds, StochasticRelaxation::activeState().tracedRays, StochasticRelaxation::activeState().tracedRays - StochasticRelaxation::activeState().numberOfMisses);
    System::err.printf(", total flux = ");
    StochasticRelaxation::activeState().totalFlux.print(&System::err);
    if ( StochasticRelaxation::activeState().importanceDriven ) {
        System::err.printf("\ntotal importance rays = %ld, total importance = %g, total area = %g",
                StochasticRelaxation::activeState().importanceTracedRays, StochasticRelaxation::activeState().totalYmp, Statistics::instance().radiance.totalArea);
    }
    System::err.printf("\n");
}

void
StochasticJacobiRadianceMethod::stchsRelaxRadDRegRadnItrtn(
    VoxelGrid *sceneWorldVoxelGrid,
    const ArrayList<Patch *> *scenePatches,
    RenderOptions *renderOptions)
{
    System::err.printf("Regular radiance iteration %d:\n", StochasticRelaxation::activeState().currentIteration);
    StochasticJacobi::doStochasticJacobiIteration(
        sceneWorldVoxelGrid,
        StochasticRelaxation::activeState().raysPerIteration,
        stchsRelaxRadElemRadn,
        NULL,
        stchsRelaxRadElemUpdRadn,
        scenePatches,
        renderOptions);

    Mcrad::mntCarloRadUpdCpuSecs();
    stchsRelaxRadPrintRegStats();
}

float
StochasticJacobiRadianceMethod::stchsRelaxRadElemImp(const StochasticRadiosityElement *elem) {
    return elem->importance;
}

void
StochasticJacobiRadianceMethod::stchsRelaxRadElemUpdImp(StochasticRadiosityElement *elem, double /*w*/) {
    double k = ((double)(StochasticRelaxation::activeState().prevImportanceTracedRays)) / ((double)(StochasticRelaxation::activeState().importanceTracedRays));

    elem->importance = ((float)(k * (elem->importance - elem->sourceImportance) + (1.0 - k) * elem->receivedImportance + elem->
                                          sourceImportance));
    elem->unShotImportance = elem->receivedImportance = 0.0;
}

void
StochasticJacobiRadianceMethod::stchsRelaxRadDRegImpItrtn(
    VoxelGrid *sceneWorldVoxelGrid,
    const ArrayList<Patch *> *scenePatches,
    RenderOptions *renderOptions)
{
    long numberOfRays;
    int doHierarchicMeshing = ElementHierarchyState::activeState().do_h_meshing;
    HierarchyClusteringMode clustering = ElementHierarchyState::activeState().clustering;
    int weighted_sampling = StochasticRelaxation::activeState().weightedSampling;
    ElementHierarchyState::activeState().do_h_meshing = false;
    ElementHierarchyState::activeState().clustering = NO_CLUSTERING;
    StochasticRelaxation::activeState().weightedSampling = false;

    numberOfRays = StochasticRelaxation::activeState().importanceRaysPerIteration;
    System::err.printf("Regular importance iteration %d:\n", StochasticRelaxation::activeState().currentIteration);

    StochasticJacobi::doStochasticJacobiIteration(
        sceneWorldVoxelGrid,
        numberOfRays,
        NULL,
        stchsRelaxRadElemImp,
        stchsRelaxRadElemUpdImp,
        scenePatches,
        renderOptions);

    Mcrad::mntCarloRadUpdCpuSecs();
    stchsRelaxRadPrintRegStats();

    ElementHierarchyState::activeState().do_h_meshing = doHierarchicMeshing;
    ElementHierarchyState::activeState().clustering = clustering;
    StochasticRelaxation::activeState().weightedSampling = weighted_sampling;
}

/**
Resets to zero all kind of things that should be reset to zero after a first
iteration of which the result only is to be used as the input of subsequent
iterations. Basically, everything that needs to be divided by the number of
rays except radiosity and importance needs to be reset to zero. This is
required for some of the experimental stuff to work
*/
void
StochasticJacobiRadianceMethod::stchsRelaxRadElemDscrdIncrm(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = ((StochasticRadiosityElement *)(element));

    if ( stochasticRadiosityElement == NULL ) {
        return;
    }

    stochasticRadiosityElement->quality = 0.0;
    stochasticRadiosityElement->traverseAllChildren(stchsRelaxRadElemDscrdIncrm);
}

void
StochasticJacobiRadianceMethod::stchsRelaxRadDscrdIncrm() {
    StochasticRelaxation::activeState().tracedRays = StochasticRelaxation::activeState().prevTracedRays = 0;

    stchsRelaxRadElemDscrdIncrm(ElementHierarchyState::activeState().topCluster);
}

bool
StochasticJacobiRadianceMethod::doStep(Scene *scene, RenderOptions *renderOptions) {
    StochasticRelaxation::setActiveState(stochasticRelaxationState);
    ElementHierarchyState::setActiveState(elementHierarchyState);
    StochasticRadiosityBasisState::setActiveState(stochasticRadiosityBasisState);
    Mcrad::monteCarloRadiosityPreStep(scene, renderOptions);

    // Do some real work now
    if ( StochasticRelaxation::activeState().currentIteration == 1 ) {
        if ( StochasticRelaxation::activeState().doNonDiffuseFirstShot ) {
            Nondiff::doNonDiffuseFirstShot(scene, this, renderOptions);
        }
        int initial_nr_of_rays = ((int)(StochasticRelaxation::activeState().tracedRays));

        if ( StochasticRelaxation::activeState().importanceDriven ) {
            if ( !StochasticRelaxation::activeState().incrementalUsesImportance ) {
                Logger::warning(NULL, "Importance is only used from the second iteration on ...");
            } else if ( StochasticRelaxation::activeState().importanceUpdated ) {
                    StochasticRelaxation::activeState().importanceUpdated = false;

                    // Propagate importance changes
                    stchsRelaxRadDIncrmImpItrtn(scene->voxelGrid, scene->patchList, renderOptions);
                    if ( StochasticRelaxation::activeState().importanceUpdatedFromScratch ) {
                        StochasticRelaxation::activeState().importanceRaysPerIteration = StochasticRelaxation::activeState().importanceTracedRays;
                    }
                }
        }
        stchsRelaxRadDIncrmRadnItrtn(scene, this, renderOptions);

        // Subsequent regular iterations will take as many rays as in the whole
        // sequence of incremental iteration steps
        StochasticRelaxation::activeState().raysPerIteration = StochasticRelaxation::activeState().tracedRays - initial_nr_of_rays;

        if ( StochasticRelaxation::activeState().discardIncremental ) {
            stchsRelaxRadDscrdIncrm();
        }
    } else {
        if ( StochasticRelaxation::activeState().importanceDriven ) {
            if ( StochasticRelaxation::activeState().importanceUpdated ) {
                StochasticRelaxation::activeState().importanceUpdated = false;

                // Propagate importance changes
                stchsRelaxRadDIncrmImpItrtn(scene->voxelGrid, scene->patchList, renderOptions);
                if ( StochasticRelaxation::activeState().importanceUpdatedFromScratch ) {
                    StochasticRelaxation::activeState().importanceRaysPerIteration = StochasticRelaxation::activeState().importanceTracedRays;
                }
            } else {
                stchsRelaxRadDRegImpItrtn(scene->voxelGrid, scene->patchList, renderOptions);
            }
        }
        stchsRelaxRadDRegRadnItrtn(scene->voxelGrid, scene->patchList, renderOptions);
    }

    stchsRelaxRadRecompDispClrs(scene->patchList);

    System::err.printf("%s\n", getStats());

    return false; // Always continue computing (never fully converged)
}
#endif
