#include <cstdlib>

#include "java/util/Formatter.h"
#include "common/RenderOptions.h"

/**
Stochastic Relaxation Radiosity (currently only stochastic Jacobi)
*/

#include "java/util/ArrayList.txx"
#include "java/lang/System.h"
#include "common/Error.h"
#include "common/statistics/Statistics.h"
#include "render/Render.h"
#include "raycasting/stochasticRaytracing/McradP.h"
#include "raycasting/stochasticRaytracing/Hierarchy.h"
#include "raycasting/stochasticRaytracing/Stochjacobi.h"
#include "raycasting/stochasticRaytracing/StochasticJacobiRadianceMethod.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

static constexpr int STRING_LENGTH = 2000;

void
StochasticJacobiRadianceMethod::appendStochasticStatsText(char *buffer, int *offset, const char *format, ...) {
    if ( *offset >= STRING_LENGTH - 1 ) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    const int available = STRING_LENGTH - *offset;
    const int written = java::Formatter::vformat(&buffer[*offset], available, format, arguments);
    va_end(arguments);

    if ( written <= 0 ) {
        return;
    }
    if ( written >= available ) {
        *offset = STRING_LENGTH - 1;
    } else {
        *offset += written;
    }
}

#ifdef RAYTRACING_ENABLED
StochasticJacobiRadianceMethod::StochasticJacobiRadianceMethod() {
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
StochasticJacobiRadianceMethod::terminate(java::ArrayList<Patch *> *scenePatches) {
    Mcrad::monteCarloRadiosityTerminate(scenePatches);
}

ColorRgb
StochasticJacobiRadianceMethod::getRadiance(Camera */*camera*/, Patch *patch, double u, double v, Vector3D dir, const RenderOptions *renderOptions) const {
    return Mcrad::monteCarloRadiosityGetRadiance(patch, u, v, dir, renderOptions);
}

Element *
StochasticJacobiRadianceMethod::createPatchData(Patch *patch) {
    return Mcrad::monteCarloRadiosityCreatePatchData(patch);
}

void
StochasticJacobiRadianceMethod::destroyPatchData(Patch *patch) {
    Mcrad::monteCarloRadiosityDestroyPatchData(patch);
}

void
StochasticJacobiRadianceMethod::writeVRML(
    const Camera * /*camera*/,
    java::OutputStream * /*outputStream*/,
    const RenderOptions * /*renderOptions*/) const
{
}

void
StochasticJacobiRadianceMethod::initialize(Scene *scene) {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.toneMapOptions = scene == nullptr ? nullptr : scene->toneMapOptions;
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.toneMapOptions == nullptr ) {
        Error::fatal(-1, "StochasticJacobiRadianceMethod::initialize", "Tone mapping context not set in scene");
    }
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.method = StochasticRaytracingMethod::STOCHASTIC_RELAXATION_RADIOSITY_METHOD;
    Mcrad::monteCarloRadiosityInit();
}

char *
StochasticJacobiRadianceMethod::getStats() {
    static char stats[STRING_LENGTH];
    int statsOffset = 0;

    appendStochasticStatsText(stats, &statsOffset, "Stochastic Relaxation Radiosity\nStatistics\n\n");
    appendStochasticStatsText(stats, &statsOffset, "Iteration nr: %d\n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration);
    appendStochasticStatsText(stats, &statsOffset, "CPU time: %g secs\n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds);
    appendStochasticStatsText(stats, &statsOffset, "%ld elements (%ld clusters, %ld surfaces)\n",
                              GLOBAL_stochasticRaytracing_hierarchy.nr_elements, GLOBAL_stochasticRaytracing_hierarchy.nr_clusters,
                              GLOBAL_stochasticRaytracing_hierarchy.nr_elements - GLOBAL_stochasticRaytracing_hierarchy.nr_clusters);
    appendStochasticStatsText(stats, &statsOffset, "Radiance rays: %ld\n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays);
    appendStochasticStatsText(stats, &statsOffset, "Importance rays: %ld\n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays);

    return stats;
}

/**
Randomly returns floor(x) or ceil(x) so that the expected value is equal to x
*/
long
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityRandomRound(float x) {
    long l = static_cast<long>(java::Math::floor(x));
    if ( drand48() < (x - static_cast<float>(l)) ) {
        l++;
    }
    return l;
}

void
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityRecomputeDisplayColors(const java::ArrayList<Patch *> *scenePatches) {
    StochasticRadiosityElement *topElement = GLOBAL_stochasticRaytracing_hierarchy.topCluster;
    if ( topElement != nullptr ) {
        topElement->traverseClusterLeafElements(StochasticRadiosityElement::stochasticRadiosityElementComputeNewVertexColors);
        topElement->traverseClusterLeafElements(StochasticRadiosityElement::stochasticRadiosityElementAdjustTVertexColors);
    } else {
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            Mcrad::monteCarloRadiosityPatchComputeNewColor(scenePatches->get(i));
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
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityQualityFactor(const StochasticRadiosityElement *elem, double w) {
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
        return w * elem->importance;
    }
    return w / StochasticRadiosityElement::stochasticRadiosityElementScalarReflectance(elem);
}

ColorRgb *
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementUnShotRadiance(const StochasticRadiosityElement *elem) {
    return elem->unShotRadiance;
}

void
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementIncrementRadiance(StochasticRadiosityElement *elem, double w) {
    // Each incremental iteration computes a different contribution to the
    // solution. The quality factor of the result remains constant
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.discardIncremental ) {
        elem->quality = 0.0;
        static bool repeated = false;
        if ( !repeated ) {
            Error::warning("stochasticRelaxationRadiosityElementIncrementRadiance",
                       "Solution of incremental Jacobi steps receives zero quality");
        }
        repeated = true;
    } else {
        elem->quality = static_cast<float>(stochasticRelaxationRadiosityQualityFactor(elem, w));
    }

    Coefficientsmcrad::stochasticRadiosityAddCoefficients(elem->radiance, elem->receivedRadiance, elem->basis);
    Coefficientsmcrad::stochasticRadiosityCopyCoefficients(elem->unShotRadiance, elem->receivedRadiance, elem->basis);
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.setSource ) {
        // Copy direct illumination and forget self emitted illumination
        elem->radiance[0] = elem->sourceRad = elem->receivedRadiance[0];
    }
    Coefficientsmcrad::stochasticRadiosityClearCoefficients(elem->receivedRadiance, elem->basis);
}

void
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityPrintIncrementalRadianceStats() {
    java::System::err.printf("%g secs., radiance rays = %ld (%ld not to background), un-shot flux = ",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays - GLOBAL_stochasticRaytracing_monteCarloRadiosityState.numberOfMisses);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.print(&java::System::err);
    java::System::err.printf(", total flux = ");
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.print(&java::System::err);
    java::System::err.printf(", indirect importance weighted un-shot flux = ");
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux.print(&java::System::err);
    java::System::err.printf("\n");
}

void
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityDoIncrementalRadianceIterations(
    Scene* scene,
    const RadianceMethod */*radianceMethod*/,
    RenderOptions *renderOptions)
{
    double refUnShot;
    long stepNumber = 0;

    int weightedSampling = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling;
    int importanceDriven = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven;
    if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance ) {
        // Temporarily switch it off
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven = false;
    }
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = false;

    stochasticRelaxationRadiosityPrintIncrementalRadianceStats();
    refUnShot = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.sumAbsComponents();
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance ) {
        refUnShot = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux.sumAbsComponents();
    }
    while ( true ) {
        // Choose nr of rays so that power carried by each ray remains equal, and
        // proportional to the number of basis functions in the rad. approx
        double unShotFraction;
        long numberOfRays;
        unShotFraction = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.sumAbsComponents() / refUnShot;
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance ) {
            unShotFraction = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux.sumAbsComponents() / refUnShot;
        }
        if ( unShotFraction < 0.01 ) {
            // Only 1/100th of self-emitted power remains un-shot
            break;
        }
        numberOfRays = stochasticRelaxationRadiosityRandomRound(
                static_cast<float>(unShotFraction * static_cast<double>(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays) *
                                   GLOBAL_stochasticRadiosity_approxDesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.
                                       approximationOrderType].basis_size));

        stepNumber++;
        java::System::err.printf("Incremental radiance propagation step %ld: %.3f%% un-shot power left.\n",
                stepNumber, 100. * unShotFraction);

        Stochjacobi::doStochasticJacobiIteration(
            scene->voxelGrid,
            numberOfRays,
            stochasticRelaxationRadiosityElementUnShotRadiance,
            nullptr,
            stochasticRelaxationRadiosityElementIncrementRadiance,
            scene->patchList,
            renderOptions);

        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.setSource = false; // Direct illumination is copied to SOURCE_FLUX(P) only the first time

        Mcrad::monteCarloRadiosityUpdateCpuSecs();
        stochasticRelaxationRadiosityPrintIncrementalRadianceStats();
        if ( unShotFraction > 0.3 ) {
            stochasticRelaxationRadiosityRecomputeDisplayColors(scene->patchList);
        }
    }

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven = importanceDriven; // Switch it back on if it was on
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = weightedSampling;
}

float
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementUnShotImportance(const StochasticRadiosityElement *elem) {
    return elem->unShotImportance;
}

void
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementIncrementImportance(StochasticRadiosityElement *elem, double /*w*/) {
    elem->importance += elem->receivedImportance;
    elem->unShotImportance = elem->receivedImportance;
    elem->receivedImportance = 0.0;
}

void
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityPrintIncrementalImportanceStats() {
    java::System::err.printf("%g secs., importance rays = %ld, un-shot importance = %g, total importance = %g, total area = %g\n",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp, Statistics::instance().radiance.totalArea);
}

void
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityDoIncrementalImportanceIterations(
    VoxelGrid *sceneWorldVoxelGrid,
    const java::ArrayList<Patch *> *scenePatches,
    RenderOptions *renderOptions)
{
    long stepNumber = 0;
    int radiance_driven = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.radianceDriven;
    int do_h_meshing = GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing;
    HierarchyClusteringMode clustering = GLOBAL_stochasticRaytracing_hierarchy.clustering;
    int weighted_sampling = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling;

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp < Numeric::EPSILON ) {
        java::System::err.printf("No source importance!!\n");
        return;
    }

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.radianceDriven = false; // Temporary switch it off
    GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing = false;
    GLOBAL_stochasticRaytracing_hierarchy.clustering = HierarchyClusteringMode::NO_CLUSTERING;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = false;

    stochasticRelaxationRadiosityPrintIncrementalRadianceStats();
    while ( true ) {
        // Choose nr of rays so that power carried by each ray is the same, and
        // proportional to the number of basis functions in the rad. approx. */
        double unShotFraction = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp / GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp;
        long numberOfRays = stochasticRelaxationRadiosityRandomRound(
                static_cast<float>(unShotFraction) * static_cast<float>(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays));
        if ( unShotFraction < 0.01 ) {
            break;
        }

        stepNumber++;
        java::System::err.printf("Incremental importance propagation step %ld: %.3f%% un-shot importance left.\n",
                stepNumber, 100.0 * unShotFraction);

        Stochjacobi::doStochasticJacobiIteration(
            sceneWorldVoxelGrid,
            numberOfRays,
            nullptr,
            stochasticRelaxationRadiosityElementUnShotImportance,
            stochasticRelaxationRadiosityElementIncrementImportance,
            scenePatches,
            renderOptions);

        Mcrad::monteCarloRadiosityUpdateCpuSecs();
        stochasticRelaxationRadiosityPrintIncrementalImportanceStats();
    }

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.radianceDriven = radiance_driven; // Switch on again
    GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing = do_h_meshing;
    GLOBAL_stochasticRaytracing_hierarchy.clustering = clustering;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = weighted_sampling;
}

ColorRgb *
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementRadiance(const StochasticRadiosityElement *elem) {
    return elem->radiance;
}

void
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementUpdateRadiance(StochasticRadiosityElement *elem, double w) {
    double k = static_cast<double>(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevTracedRays) / static_cast<double>(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays > 0
                   ? GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays
                   : 1);

    if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.naiveMerging ) {
        double quality = stochasticRelaxationRadiosityQualityFactor(elem, w);
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
        elem->quality += static_cast<float>(quality); // Add quality
    }

    // Subtract source radiosity
    elem->radiance[0].subtract(elem->radiance[0], elem->sourceRad);

    // Combine with previous results
    Coefficientsmcrad::stochasticRadiosityScaleCoefficients(static_cast<float>(k), elem->radiance, elem->basis);
    Coefficientsmcrad::stochasticRadiosityScaleCoefficients((1.0f - static_cast<float>(k)), elem->receivedRadiance, elem->basis);
    Coefficientsmcrad::stochasticRadiosityAddCoefficients(elem->radiance, elem->receivedRadiance, elem->basis);

    // Re-add source radiosity
    elem->radiance[0].add(elem->radiance[0], elem->sourceRad);

    // Clear un-shot and received radiance
    Coefficientsmcrad::stochasticRadiosityClearCoefficients(elem->unShotRadiance, elem->basis);
    Coefficientsmcrad::stochasticRadiosityClearCoefficients(elem->receivedRadiance, elem->basis);
}

void
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityPrintRegularStats() {
    java::System::err.printf("%g secs., radiance rays = %ld (%ld not to background), un-shot flux = ",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays - GLOBAL_stochasticRaytracing_monteCarloRadiosityState.numberOfMisses);
    java::System::err.printf(", total flux = ");
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.print(&java::System::err);
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
        java::System::err.printf("\ntotal importance rays = %ld, total importance = %g, GLOBAL_statistics_totalArea = %g",
                GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp, Statistics::instance().radiance.totalArea);
    }
    java::System::err.printf("\n");
}

void
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityDoRegularRadianceIteration(
    VoxelGrid *sceneWorldVoxelGrid,
    const java::ArrayList<Patch *> *scenePatches,
    RenderOptions *renderOptions)
{
    java::System::err.printf("Regular radiance iteration %d:\n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration);
    Stochjacobi::doStochasticJacobiIteration(
        sceneWorldVoxelGrid,
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.raysPerIteration,
        stochasticRelaxationRadiosityElementRadiance,
        nullptr,
        stochasticRelaxationRadiosityElementUpdateRadiance,
        scenePatches,
        renderOptions);

    Mcrad::monteCarloRadiosityUpdateCpuSecs();
    stochasticRelaxationRadiosityPrintRegularStats();
}

float
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementImportance(const StochasticRadiosityElement *elem) {
    return elem->importance;
}

void
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementUpdateImportance(StochasticRadiosityElement *elem, double /*w*/) {
    double k = static_cast<double>(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevImportanceTracedRays) / static_cast<double>(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays);

    elem->importance = static_cast<float>(k * (elem->importance - elem->sourceImportance) + (1.0 - k) * elem->receivedImportance + elem->
                                          sourceImportance);
    elem->unShotImportance = elem->receivedImportance = 0.0;
}

void
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityDoRegularImportanceIteration(
    VoxelGrid *sceneWorldVoxelGrid,
    const java::ArrayList<Patch *> *scenePatches,
    RenderOptions *renderOptions)
{
    long numberOfRays;
    int doHierarchicMeshing = GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing;
    HierarchyClusteringMode clustering = GLOBAL_stochasticRaytracing_hierarchy.clustering;
    int weighted_sampling = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling;
    GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing = false;
    GLOBAL_stochasticRaytracing_hierarchy.clustering = HierarchyClusteringMode::NO_CLUSTERING;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = false;

    numberOfRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceRaysPerIteration;
    java::System::err.printf("Regular importance iteration %d:\n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration);

    Stochjacobi::doStochasticJacobiIteration(
        sceneWorldVoxelGrid,
        numberOfRays,
        nullptr,
        stochasticRelaxationRadiosityElementImportance,
        stochasticRelaxationRadiosityElementUpdateImportance,
        scenePatches,
        renderOptions);

    Mcrad::monteCarloRadiosityUpdateCpuSecs();
    stochasticRelaxationRadiosityPrintRegularStats();

    GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing = doHierarchicMeshing;
    GLOBAL_stochasticRaytracing_hierarchy.clustering = clustering;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = weighted_sampling;
}

/**
Resets to zero all kind of things that should be reset to zero after a first
iteration of which the result only is to be used as the input of subsequent
iterations. Basically, everything that needs to be divided by the number of
rays except radiosity and importance needs to be reset to zero. This is
required for some of the experimental stuff to work
*/
void
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityElementDiscardIncremental(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = static_cast<StochasticRadiosityElement *>(element);

    if ( stochasticRadiosityElement == nullptr ) {
        return;
    }

    stochasticRadiosityElement->quality = 0.0;
    stochasticRadiosityElement->traverseAllChildren(stochasticRelaxationRadiosityElementDiscardIncremental);
}

void
StochasticJacobiRadianceMethod::stochasticRelaxationRadiosityDiscardIncremental() {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevTracedRays = 0;

    stochasticRelaxationRadiosityElementDiscardIncremental(GLOBAL_stochasticRaytracing_hierarchy.topCluster);
}

bool
StochasticJacobiRadianceMethod::doStep(Scene *scene, RenderOptions *renderOptions) {
    Mcrad::monteCarloRadiosityPreStep(scene, renderOptions);

    // Do some real work now
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration == 1 ) {
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.doNonDiffuseFirstShot ) {
            Nondiff::doNonDiffuseFirstShot(scene, this, renderOptions);
        }
        int initial_nr_of_rays = static_cast<int>(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays);

        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
            if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance ) {
                Error::warning(nullptr, "Importance is only used from the second iteration on ...");
            } else if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdated ) {
                    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdated = false;

                    // Propagate importance changes
                    stochasticRelaxationRadiosityDoIncrementalImportanceIterations(scene->voxelGrid, scene->patchList, renderOptions);
                    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch ) {
                        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceRaysPerIteration = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays;
                    }
                }
        }
        stochasticRelaxationRadiosityDoIncrementalRadianceIterations(scene, this, renderOptions);

        // Subsequent regular iterations will take as many rays as in the whole
        // sequence of incremental iteration steps
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.raysPerIteration = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays - initial_nr_of_rays;

        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.discardIncremental ) {
            stochasticRelaxationRadiosityDiscardIncremental();
        }
    } else {
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
            if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdated ) {
                GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdated = false;

                // Propagate importance changes
                stochasticRelaxationRadiosityDoIncrementalImportanceIterations(scene->voxelGrid, scene->patchList, renderOptions);
                if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch ) {
                    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceRaysPerIteration = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays;
                }
            } else {
                stochasticRelaxationRadiosityDoRegularImportanceIteration(scene->voxelGrid, scene->patchList, renderOptions);
            }
        }
        stochasticRelaxationRadiosityDoRegularRadianceIteration(scene->voxelGrid, scene->patchList, renderOptions);
    }

    stochasticRelaxationRadiosityRecomputeDisplayColors(scene->patchList);

    java::System::err.printf("%s\n", getStats());

    return false; // Always continue computing (never fully converged)
}
#endif
