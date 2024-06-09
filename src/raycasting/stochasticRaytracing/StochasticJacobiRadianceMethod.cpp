/**
Stochastic Relaxation Radiosity (currently only stochastic Jacobi)
*/

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/Statistics.h"
#include "render/render.h"
#include "render/opengl.h"
#include "raycasting/common/Raytracer.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "raycasting/stochasticRaytracing/stochjacobi.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "raycasting/stochasticRaytracing/StochasticJacobiRadianceMethod.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracingState.h"

#define STRING_LENGTH 2000

#ifdef RAYTRACING_ENABLED
StochasticJacobiRadianceMethod::StochasticJacobiRadianceMethod() {
    monteCarloRadiosityDefaults();
    className = STOCHASTIC_JACOBI;
}

StochasticJacobiRadianceMethod::~StochasticJacobiRadianceMethod() {
}

const char *
StochasticJacobiRadianceMethod::getRadianceMethodName() const {
    return "Stochastic Jacobi";
}

void
StochasticJacobiRadianceMethod::parseOptions(int *argc, char **argv) {
}

void
StochasticJacobiRadianceMethod::terminate(java::ArrayList<Patch *> *scenePatches) {
    monteCarloRadiosityTerminate(scenePatches);
}

ColorRgb
StochasticJacobiRadianceMethod::getRadiance(Camera *camera, Patch *patch, double u, double v, Vector3D dir, const RenderOptions *renderOptions) const {
    return monteCarloRadiosityGetRadiance(patch, u, v, dir, renderOptions);
}

Element *
StochasticJacobiRadianceMethod::createPatchData(Patch *patch) {
    return monteCarloRadiosityCreatePatchData(patch);
}

void
StochasticJacobiRadianceMethod::destroyPatchData(Patch *patch) {
    monteCarloRadiosityDestroyPatchData(patch);
}

void
StochasticJacobiRadianceMethod::writeVRML(const Camera * /*camera*/, FILE *fp, const RenderOptions * /*renderOptions*/) const {
}

void
StochasticJacobiRadianceMethod::initialize(Scene *scene) {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.method = StochasticRaytracingMethod::STOCHASTIC_RELAXATION_RADIOSITY_METHOD;
    monteCarloRadiosityInit();
}

char *
StochasticJacobiRadianceMethod::getStats() {
    static char stats[STRING_LENGTH];
    char *p;
    int n;

    p = stats;
    snprintf(p, STRING_LENGTH, "Stochastic Relaxation Radiosity\nStatistics\n\n%n", &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Iteration nr: %d\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "CPU time: %g secs\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, &n);
    p += n;

    snprintf(p, STRING_LENGTH, "%ld elements (%ld clusters, %ld surfaces)\n%n",
             GLOBAL_stochasticRaytracing_hierarchy.nr_elements, GLOBAL_stochasticRaytracing_hierarchy.nr_clusters, GLOBAL_stochasticRaytracing_hierarchy.nr_elements - GLOBAL_stochasticRaytracing_hierarchy.nr_clusters, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Radiance rays: %ld\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays, &n);
    p += n;
    snprintf(p, STRING_LENGTH, "Importance rays: %ld\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays, &n);

    return stats;
}

/**
Randomly returns floor(x) or ceil(x) so that the expected value is equal to x
*/
static long
stochasticRelaxationRadiosityRandomRound(float x) {
    long l = (long)java::Math::floor(x);
    if ( drand48() < (x - (float) l) ) {
        l++;
    }
    return l;
}

static void
stochasticRelaxationRadiosityRecomputeDisplayColors(const java::ArrayList<Patch *> *scenePatches) {
    StochasticRadiosityElement *topElement = GLOBAL_stochasticRaytracing_hierarchy.topCluster;
    if ( topElement != nullptr ) {
        topElement->traverseClusterLeafElements(stochasticRadiosityElementComputeNewVertexColors);
        topElement->traverseClusterLeafElements(stochasticRadiosityElementAdjustTVertexColors);
    } else {
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            monteCarloRadiosityPatchComputeNewColor(scenePatches->get(i));
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
static double
stochasticRelaxationRadiosityQualityFactor(const StochasticRadiosityElement *elem, double w) {
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
        return w * elem->importance;
    }
    return w / stochasticRadiosityElementScalarReflectance(elem);
}

static ColorRgb *
stochasticRelaxationRadiosityElementUnShotRadiance(const StochasticRadiosityElement *elem) {
    return elem->unShotRadiance;
}

static void
stochasticRelaxationRadiosityElementIncrementRadiance(StochasticRadiosityElement *elem, double w) {
    // Each incremental iteration computes a different contribution to the
    // solution. The quality factor of the result remains constant
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.discardIncremental ) {
        elem->quality = 0.0;
        static bool repeated = false;
        if ( !repeated ) {
            logWarning("stochasticRelaxationRadiosityElementIncrementRadiance",
                       "Solution of incremental Jacobi steps receives zero quality");
        }
        repeated = true;
    } else {
        elem->quality = (float)stochasticRelaxationRadiosityQualityFactor(elem, w);
    }

    stochasticRadiosityAddCoefficients(elem->radiance, elem->receivedRadiance, elem->basis);
    stochasticRadiosityCopyCoefficients(elem->unShotRadiance, elem->receivedRadiance, elem->basis);
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.setSource ) {
        // Copy direct illumination and forget self emitted illumination
        elem->radiance[0] = elem->sourceRad = elem->receivedRadiance[0];
    }
    stochasticRadiosityClearCoefficients(elem->receivedRadiance, elem->basis);
}

static void
stochasticRelaxationRadiosityPrintIncrementalRadianceStats() {
    fprintf(stderr, "%g secs., radiance rays = %ld (%ld not to background), un-shot flux = ",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays - GLOBAL_stochasticRaytracing_monteCarloRadiosityState.numberOfMisses);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.print(stderr);
    fprintf(stderr, ", total flux = ");
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.print(stderr);
    fprintf(stderr, ", indirect importance weighted un-shot flux = ");
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux.print(stderr);
    fprintf(stderr, "\n");
}

static void
stochasticRelaxationRadiosityDoIncrementalRadianceIterations(
    Scene* scene,
    const RadianceMethod *radianceMethod,
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
                (float)(unShotFraction * (double)GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays *
                        GLOBAL_stochasticRadiosity_approxDesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size));

        stepNumber++;
        fprintf(stderr, "Incremental radiance propagation step %ld: %.3f%% un-shot power left.\n",
                stepNumber, 100. * unShotFraction);

        doStochasticJacobiIteration(
            scene->voxelGrid,
            numberOfRays,
            stochasticRelaxationRadiosityElementUnShotRadiance,
            nullptr,
            stochasticRelaxationRadiosityElementIncrementRadiance,
            scene->patchList,
            renderOptions);

        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.setSource = false; // Direct illumination is copied to SOURCE_FLUX(P) only the first time

        monteCarloRadiosityUpdateCpuSecs();
        stochasticRelaxationRadiosityPrintIncrementalRadianceStats();
        if ( unShotFraction > 0.3 ) {
            stochasticRelaxationRadiosityRecomputeDisplayColors(scene->patchList);

            if ( GLOBAL_rayTracer != nullptr ) {
                // TODO: Verify this is not needed, has been disabled flow on May 29 2024
                //openGlRenderNewDisplayList(scene->clusteredRootGeometry, renderOptions);
                //openGlRenderScene(scene, GLOBAL_rayTracer, radianceMethod, renderOptions);
            }
        }
    }

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven = importanceDriven; // Switch it back on if it was on
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = weightedSampling;
}

static float
stochasticRelaxationRadiosityElementUnShotImportance(const StochasticRadiosityElement *elem) {
    return elem->unShotImportance;
}

static void
stochasticRelaxationRadiosityElementIncrementImportance(StochasticRadiosityElement *elem, double /*w*/) {
    elem->importance += elem->receivedImportance;
    elem->unShotImportance = elem->receivedImportance;
    elem->receivedImportance = 0.0;
}

static void
stochasticRelaxationRadiosityPrintIncrementalImportanceStats() {
    fprintf(stderr, "%g secs., importance rays = %ld, un-shot importance = %g, total importance = %g, total area = %g\n",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp, GLOBAL_statistics.totalArea);
}

static void
stochasticRelaxationRadiosityDoIncrementalImportanceIterations(
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
        fprintf(stderr, "No source importance!!\n");
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
                (float)unShotFraction * (float) GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays);
        if ( unShotFraction < 0.01 ) {
            break;
        }

        stepNumber++;
        fprintf(stderr, "Incremental importance propagation step %ld: %.3f%% un-shot importance left.\n",
                stepNumber, 100.0 * unShotFraction);

        doStochasticJacobiIteration(
            sceneWorldVoxelGrid,
            numberOfRays,
            nullptr,
            stochasticRelaxationRadiosityElementUnShotImportance,
            stochasticRelaxationRadiosityElementIncrementImportance,
            scenePatches,
            renderOptions);

        monteCarloRadiosityUpdateCpuSecs();
        stochasticRelaxationRadiosityPrintIncrementalImportanceStats();
    }

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.radianceDriven = radiance_driven; // Switch on again
    GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing = do_h_meshing;
    GLOBAL_stochasticRaytracing_hierarchy.clustering = clustering;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = weighted_sampling;
}

static ColorRgb *
stochasticRelaxationRadiosityElementRadiance(const StochasticRadiosityElement *elem) {
    return elem->radiance;
}

static void
stochasticRelaxationRadiosityElementUpdateRadiance(StochasticRadiosityElement *elem, double w) {
    double k = (double) GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevTracedRays / (double) (GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays > 0 ? GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays : 1);

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
        elem->quality += (float)quality; // Add quality
    }

    // Subtract source radiosity
    elem->radiance[0].subtract(elem->radiance[0], elem->sourceRad);

    // Combine with previous results
    stochasticRadiosityScaleCoefficients((float)k, elem->radiance, elem->basis);
    stochasticRadiosityScaleCoefficients((1.0f - (float)k), elem->receivedRadiance, elem->basis);
    stochasticRadiosityAddCoefficients(elem->radiance, elem->receivedRadiance, elem->basis);

    // Re-add source radiosity
    elem->radiance[0].add(elem->radiance[0], elem->sourceRad);

    // Clear un-shot and received radiance
    stochasticRadiosityClearCoefficients(elem->unShotRadiance, elem->basis);
    stochasticRadiosityClearCoefficients(elem->receivedRadiance, elem->basis);
}

static void
stochasticRelaxationRadiosityPrintRegularStats() {
    fprintf(stderr, "%g secs., radiance rays = %ld (%ld not to background), un-shot flux = ",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays - GLOBAL_stochasticRaytracing_monteCarloRadiosityState.numberOfMisses);
    fprintf(stderr, ", total flux = ");
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.print(stderr);
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
        fprintf(stderr, "\ntotal importance rays = %ld, total importance = %g, GLOBAL_statistics_totalArea = %g",
                GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp, GLOBAL_statistics.totalArea);
    }
    fprintf(stderr, "\n");
}

static void
stochasticRelaxationRadiosityDoRegularRadianceIteration(
    VoxelGrid *sceneWorldVoxelGrid,
    const java::ArrayList<Patch *> *scenePatches,
    RenderOptions *renderOptions)
{
    fprintf(stderr, "Regular radiance iteration %d:\n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration);
    doStochasticJacobiIteration(
        sceneWorldVoxelGrid,
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.raysPerIteration,
        stochasticRelaxationRadiosityElementRadiance,
        nullptr,
        stochasticRelaxationRadiosityElementUpdateRadiance,
        scenePatches,
        renderOptions);

    monteCarloRadiosityUpdateCpuSecs();
    stochasticRelaxationRadiosityPrintRegularStats();
}

static float
stochasticRelaxationRadiosityElementImportance(const StochasticRadiosityElement *elem) {
    return elem->importance;
}

static void
stochasticRelaxationRadiosityElementUpdateImportance(StochasticRadiosityElement *elem, double /*w*/) {
    double k = (double) GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevImportanceTracedRays / (double) GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays;

    elem->importance = (float)(k * (elem->importance - elem->sourceImportance) + (1.0 - k) * elem->receivedImportance + elem->sourceImportance);
    elem->unShotImportance = elem->receivedImportance = 0.0;
}

static void
stochasticRelaxationRadiosityDoRegularImportanceIteration(
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
    fprintf(stderr, "Regular importance iteration %d:\n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration);

    doStochasticJacobiIteration(
        sceneWorldVoxelGrid,
        numberOfRays,
        nullptr,
        stochasticRelaxationRadiosityElementImportance,
        stochasticRelaxationRadiosityElementUpdateImportance,
        scenePatches,
        renderOptions);

    monteCarloRadiosityUpdateCpuSecs();
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
static void
stochasticRelaxationRadiosityElementDiscardIncremental(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = (StochasticRadiosityElement *)element;

    if ( stochasticRadiosityElement == nullptr ) {
        return;
    }

    stochasticRadiosityElement->quality = 0.0;
    stochasticRadiosityElement->traverseAllChildren(stochasticRelaxationRadiosityElementDiscardIncremental);
}

static void
stochasticRelaxationRadiosityDiscardIncremental() {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevTracedRays = 0;

    stochasticRelaxationRadiosityElementDiscardIncremental(GLOBAL_stochasticRaytracing_hierarchy.topCluster);
}

static void
stochasticRelaxationRadiosityRenderPatch(const Patch *patch, const Camera *camera, const RenderOptions *renderOptions) {
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited ) {
        topLevelStochasticRadiosityElement(patch)->traverseQuadTreeLeafs(stochasticRadiosityElementRender, renderOptions);
    } else {
        // Not yet initialized
        openGlRenderPatchCallBack(patch, camera, renderOptions);
    }
}

void
StochasticJacobiRadianceMethod::renderScene(const Scene *scene, const RenderOptions *renderOptions) const {
    if ( renderOptions->frustumCulling ) {
        openGlRenderWorldOctree(
            scene,
            stochasticRelaxationRadiosityRenderPatch,
            renderOptions);
    } else {
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            stochasticRelaxationRadiosityRenderPatch(scene->patchList->get(i), scene->camera, renderOptions);
        }
    }
}

bool
StochasticJacobiRadianceMethod::doStep(Scene *scene, RenderOptions *renderOptions) {
    monteCarloRadiosityPreStep(scene, renderOptions);

    // Do some real work now
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration == 1 ) {
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.doNonDiffuseFirstShot ) {
            doNonDiffuseFirstShot(scene, this, renderOptions);
        }
        int initial_nr_of_rays = (int)GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays;

        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
            if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance ) {
                logWarning(nullptr, "Importance is only used from the second iteration on ...");
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

    fprintf(stderr, "%s\n", getStats());

    return false; // Always continue computing (never fully converged)
}
#endif
