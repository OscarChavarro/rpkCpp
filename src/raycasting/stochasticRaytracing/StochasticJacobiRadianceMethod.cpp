/**
Stochastic Relaxation Radiosity (currently only stochastic Jacobi)
*/

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "material/statistics.h"
#include "render/render.h"
#include "scene/scene.h"
#include "render/opengl.h"
#include "raycasting/common/Raytracer.h"
#include "raycasting/stochasticRaytracing/vrml/vrml.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "raycasting/stochasticRaytracing/stochjacobi.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "raycasting/stochasticRaytracing/StochasticJacobiRadianceMethod.h"

StochasticJacobiRadianceMethod::StochasticJacobiRadianceMethod() {
}

StochasticJacobiRadianceMethod::~StochasticJacobiRadianceMethod() {
}

void
StochasticJacobiRadianceMethod::defaultValues() {
}

void
StochasticJacobiRadianceMethod::parseOptions(int *argc, char **argv) {
}

void
StochasticJacobiRadianceMethod::initialize(java::ArrayList<Patch *> *scenePatches) {

}

void
StochasticJacobiRadianceMethod::terminate(java::ArrayList<Patch *> *scenePatches) {
    monteCarloRadiosityTerminate(scenePatches);
}

COLOR
StochasticJacobiRadianceMethod::getRadiance(Patch *patch, double u, double v, Vector3D dir) {
    return monteCarloRadiosityGetRadiance(patch, u, v, dir);
}

Element *
StochasticJacobiRadianceMethod::createPatchData(Patch *patch) {
    return monteCarloRadiosityCreatePatchData(patch);
}

void
StochasticJacobiRadianceMethod::destroyPatchData(Patch *patch) {
    monteCarloRadiosityDestroyPatchData(patch);
}

char *
StochasticJacobiRadianceMethod::getStats() {
    return nullptr;
}

void
StochasticJacobiRadianceMethod::renderScene(java::ArrayList<Patch *> *scenePatches) {
}

void
StochasticJacobiRadianceMethod::writeVRML(FILE *fp){
}

static void
stochasticRelaxationRadiosityInit(java::ArrayList<Patch *> * /*scenePatches*/) {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.method = STOCHASTIC_RELAXATION_RADIOSITY_METHOD;
    monteCarloRadiosityInit();
}

#define STRING_SIZE 2000

static char *
stochasticRelaxationRadiosityGetStats() {
    static char stats[STRING_SIZE];
    char *p;
    int n;

    p = stats;
    snprintf(p, STRING_SIZE, "Stochastic Relaxation Radiosity\nStatistics\n\n%n", &n);
    p += n;
    snprintf(p, STRING_SIZE, "Iteration nr: %d\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration, &n);
    p += n;
    snprintf(p, STRING_SIZE, "CPU time: %g secs\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, &n);
    p += n;

    snprintf(p, STRING_SIZE, "%ld elements (%ld clusters, %ld surfaces)\n%n",
            GLOBAL_stochasticRaytracing_hierarchy.nr_elements, GLOBAL_stochasticRaytracing_hierarchy.nr_clusters, GLOBAL_stochasticRaytracing_hierarchy.nr_elements - GLOBAL_stochasticRaytracing_hierarchy.nr_clusters, &n);
    p += n;
    snprintf(p, STRING_SIZE, "Radiance rays: %ld\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays, &n);
    p += n;
    snprintf(p, STRING_SIZE, "Importance rays: %ld\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays, &n);

    return stats;
}

/**
Randomly returns floor(x) or ceil(x) so that the expected value is equal to x
*/
static long
stochasticRelaxationRadiosityRandomRound(float x) {
    long l = (long)std::floor(x);
    if ( drand48() < (x - (float) l)) {
        l++;
    }
    return l;
}

static void
stochasticRelaxationRadiosityRecomputeDisplayColors(java::ArrayList<Patch *> *scenePatches) {
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
Computes quality factor on given leaf element (see PhD Ph.Bekaert p.152).
In the basic algorithms by Neumann et al. the quality factor would
correspond to the inverse of the elementary ray power. The quality factor
indicates the quality of the radiosity solution on a given leaf element.
The quality factor after different iterations is additive. It is used in order
to properly merge the result of new iterations with the result of previous
iterations properly taking into account the number of rays and importance
distribution
*/
static double
stochasticRelaxationRadiosityQualityFactor(StochasticRadiosityElement *elem, double w) {
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
        return w * elem->importance;
    }
    return w / stochasticRadiosityElementScalarReflectance(elem);
}

static COLOR *
stochasticRelaxationRadiosityElementUnShotRadiance(StochasticRadiosityElement *elem) {
    return elem->unShotRadiance;
}

static void
stochasticRelaxationRadiosityElementIncrementRadiance(StochasticRadiosityElement *elem, double w) {
    /* Each incremental iteration computes a different contribution to the
     * solution. The quality factor of the result remains constant. */
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.discardIncremental ) {
        elem->quality = 0.0;
        {
            static int wgiv = false;
            if ( !wgiv ) {
                logWarning("stochasticRelaxationRadiosityElementIncrementRadiance", "Solution of incremental Jacobi steps receives zero quality");
            }
            wgiv = true;
        }
    } else {
        elem->quality = (float)stochasticRelaxationRadiosityQualityFactor(elem, w);
    }

    stochasticRadiosityAddCoefficients(elem->radiance, elem->receivedRadiance, elem->basis);
    stochasticRadiosityCopyCoefficients(elem->unShotRadiance, elem->receivedRadiance, elem->basis);
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.setSource ) {
        /* copy direct illumination and forget selfemitted illumination */
        elem->radiance[0] = elem->sourceRad = elem->receivedRadiance[0];
    }
    stochasticRadiosityClearCoefficients(elem->receivedRadiance, elem->basis);
}

static void
stochasticRelaxationRadiosityPrintIncrementalRadianceStats() {
    fprintf(stderr, "%g secs., radiance rays = %ld (%ld not to background), unshot flux = ",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays - GLOBAL_stochasticRaytracing_monteCarloRadiosityState.numberOfMisses);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.print(stderr);
    fprintf(stderr, ", total flux = ");
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.print(stderr);
    fprintf(stderr, ", indirect importance weighted unshot flux = ");
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux.print(stderr);
    fprintf(stderr, "\n");
}

static void
stochasticRelaxationRadiosityDoIncrementalRadianceIterations(java::ArrayList<Patch *> *scenePatches) {
    double refUnShot;
    long stepNumber = 0;

    int weighted_sampling = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling;
    int importance_driven = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven;
    if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance ) {
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven = false;
    } /* temporarily switch it off */
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = false;

    stochasticRelaxationRadiosityPrintIncrementalRadianceStats();
    refUnShot = colorSumAbsComponents(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux);
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance ) {
        refUnShot = colorSumAbsComponents(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux);
    }
    while ( true ) {
        // Choose nr of rays so that power carried by each ray remains equal, and
        // proportional to the number of basis functions in the rad. approx
        double unShotFraction;
        long nr_rays;
        unShotFraction = colorSumAbsComponents(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux) / refUnShot;
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance ) {
            unShotFraction = colorSumAbsComponents(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux) / refUnShot;
        }
        if ( unShotFraction < 0.01 ) {
            // Only 1/100th of self-emitted power remains un-shot
            break;
        }
        nr_rays = stochasticRelaxationRadiosityRandomRound(
                (float)(unShotFraction * (double)GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays *
                        GLOBAL_stochasticRadiosity_approxDesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size));

        stepNumber++;
        fprintf(stderr, "Incremental radiance propagation step %ld: %.3f%% unshot power left.\n",
                stepNumber, 100. * unShotFraction);

        doStochasticJacobiIteration(nr_rays, stochasticRelaxationRadiosityElementUnShotRadiance, nullptr,
                                    stochasticRelaxationRadiosityElementIncrementRadiance, scenePatches);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.setSource = false; // Direct illumination is copied to SOURCE_FLUX(P) only the first time

        monteCarloRadiosityUpdateCpuSecs();
        stochasticRelaxationRadiosityPrintIncrementalRadianceStats();
        if ( unShotFraction > 0.3 ) {
            stochasticRelaxationRadiosityRecomputeDisplayColors(scenePatches);
            openGlRenderNewDisplayList();

            int (*f)() = nullptr;
            if ( GLOBAL_raytracer_activeRaytracer != nullptr ) {
                f = GLOBAL_raytracer_activeRaytracer->Redisplay;
            }

            openGlRenderScene(scenePatches, GLOBAL_scene_clusteredGeometries, f);
        }
    }

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven = importance_driven;    /* switch it back on if it was on */
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = weighted_sampling;
}

static float
stochasticRelaxationRadiosityElementUnShotImportance(StochasticRadiosityElement *elem) {
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
    fprintf(stderr, "%g secs., importance rays = %ld, unshot importance = %g, total importance = %g, total area = %g\n",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp, GLOBAL_statistics.totalArea);
}

static void
stochasticRelaxationRadiosityDoIncrementalImportanceIterations(java::ArrayList<Patch *> *scenePatches) {
    long step_nr = 0;
    int radiance_driven = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.radianceDriven;
    int do_h_meshing = GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing;
    CLUSTERING_MODE clustering = GLOBAL_stochasticRaytracing_hierarchy.clustering;
    int weighted_sampling = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling;

    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp < EPSILON ) {
        fprintf(stderr, "No source importance!!\n");
        return;
    }

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.radianceDriven = false;    /* temporary switch it off */
    GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing = false;
    GLOBAL_stochasticRaytracing_hierarchy.clustering = NO_CLUSTERING;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = false;

    stochasticRelaxationRadiosityPrintIncrementalRadianceStats();
    while ( true ) {
        // Choose nr of rays so that power carried by each ray is the same, and
        // proportional to the number of basis functions in the rad. approx. */
        double unshot_fraction = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp / GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp;
        long nr_rays = stochasticRelaxationRadiosityRandomRound(
                (float)unshot_fraction * (float) GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays);
        if ( unshot_fraction < 0.01 ) {
            break;
        }

        step_nr++;
        fprintf(stderr, "Incremental importance propagation step %ld: %.3f%% un-shot importance left.\n",
                step_nr, 100.0 * unshot_fraction);

        doStochasticJacobiIteration(nr_rays, nullptr, stochasticRelaxationRadiosityElementUnShotImportance,
                                    stochasticRelaxationRadiosityElementIncrementImportance, scenePatches);

        monteCarloRadiosityUpdateCpuSecs();
        stochasticRelaxationRadiosityPrintIncrementalImportanceStats();
    }

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.radianceDriven = radiance_driven; // Switch on again
    GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing = do_h_meshing;
    GLOBAL_stochasticRaytracing_hierarchy.clustering = clustering;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = weighted_sampling;
}

static COLOR *
stochasticRelaxationRadiosityElementRadiance(StochasticRadiosityElement *elem) {
    return elem->radiance;
}

static void
stochasticRelaxationRadiosityElementUpdateRadiance(StochasticRadiosityElement *elem, double w) {
    double k = (double) GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevTracedRays / (double) (GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays > 0 ? GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays : 1);

    if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.naiveMerging ) {
        double quality = stochasticRelaxationRadiosityQualityFactor(elem, w);
        if ( elem->quality < EPSILON ) {
            // Solution of this iteration takes over
            k = 0.0;
        } else if ( quality < EPSILON ) {
            // Keep result of previous iterations
            k = 1.0;
        } else if ( elem->quality + quality > EPSILON ) {
            k = elem->quality / (elem->quality + quality);
        } else {
            // Quality of new solution is so high that it must take over
            k = 0.0;
        }
        elem->quality += (float)quality; // Add quality
    }

    // Subtract source radiosity
    colorSubtract(elem->radiance[0], elem->sourceRad, elem->radiance[0]);

    // Combine with previous results
    stochasticRadiosityScaleCoefficients((float)k, elem->radiance, elem->basis);
    stochasticRadiosityScaleCoefficients((1.0f - (float)k), elem->receivedRadiance, elem->basis);
    stochasticRadiosityAddCoefficients(elem->radiance, elem->receivedRadiance, elem->basis);

    // Re-add source radiosity
    colorAdd(elem->radiance[0], elem->sourceRad, elem->radiance[0]);

    // Clear un-shot and received radiance
    stochasticRadiosityClearCoefficients(elem->unShotRadiance, elem->basis);
    stochasticRadiosityClearCoefficients(elem->receivedRadiance, elem->basis);
}

static void
stochasticRelaxationRadiosityPrintRegularStats() {
    fprintf(stderr, "%g secs., radiance rays = %ld (%ld not to background), unshot flux = ",
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
stochasticRelaxationRadiosityDoRegularRadianceIteration(java::ArrayList<Patch *> *scenePatches) {
    fprintf(stderr, "Regular radiance iteration %d:\n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration);
    doStochasticJacobiIteration(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.raysPerIteration,
                                stochasticRelaxationRadiosityElementRadiance, nullptr,
                                stochasticRelaxationRadiosityElementUpdateRadiance, scenePatches);

    monteCarloRadiosityUpdateCpuSecs();
    stochasticRelaxationRadiosityPrintRegularStats();
}

static float
stochasticRelaxationRadiosityElementImportance(StochasticRadiosityElement *elem) {
    return elem->importance;
}

static void
stochasticRelaxationRadiosityElementUpdateImportance(StochasticRadiosityElement *elem, double /*w*/) {
    double k = (double) GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevImportanceTracedRays / (double) GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays;

    elem->importance = (float)(k * (elem->importance - elem->sourceImportance) + (1.0 - k) * elem->receivedImportance + elem->sourceImportance);
    elem->unShotImportance = elem->receivedImportance = 0.0;
}

static void
stochasticRelaxationRadiosityDoRegularImportanceIteration(java::ArrayList<Patch *> *scenePatches) {
    long nr_rays;
    int do_h_meshing = GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing;
    CLUSTERING_MODE clustering = GLOBAL_stochasticRaytracing_hierarchy.clustering;
    int weighted_sampling = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling;
    GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing = false;
    GLOBAL_stochasticRaytracing_hierarchy.clustering = NO_CLUSTERING;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = false;

    nr_rays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceRaysPerIteration;
    fprintf(stderr, "Regular importance iteration %d:\n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration);
    doStochasticJacobiIteration(nr_rays, nullptr, stochasticRelaxationRadiosityElementImportance,
                                stochasticRelaxationRadiosityElementUpdateImportance, scenePatches);

    monteCarloRadiosityUpdateCpuSecs();
    stochasticRelaxationRadiosityPrintRegularStats();

    GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing = do_h_meshing;
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
stochasticRelaxationRadiosityRenderPatch(Patch *patch) {
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited ) {
        topLevelGalerkinElement(patch)->traverseQuadTreeLeafs(stochasticRadiosityElementRender);
    } else {
        // Not yet initialized
        openGlRenderPatch(patch);
    }
}

static void
stochasticRelaxationRadiosityRender(java::ArrayList<Patch *> *scenePatches) {
    if ( GLOBAL_render_renderOptions.frustumCulling ) {
        openGlRenderWorldOctree(stochasticRelaxationRadiosityRenderPatch);
    } else {
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            stochasticRelaxationRadiosityRenderPatch(scenePatches->get(i));
        }
    }
}

int
StochasticJacobiRadianceMethod::doStep(java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches) {
    monteCarloRadiosityPreStep(scenePatches);

    // Do some real work now
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration == 1 ) {
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.doNonDiffuseFirstShot ) {
            doNonDiffuseFirstShot(scenePatches, lightPatches);
        }
        int initial_nr_of_rays = (int)GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays;

        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
            if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance ) {
                logWarning(nullptr, "Importance is only used from the second iteration on ...");
            } else if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdated ) {
                    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdated = false;

                    // Propagate importance changes
                    stochasticRelaxationRadiosityDoIncrementalImportanceIterations(scenePatches);
                    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch ) {
                        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceRaysPerIteration = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays;
                    }
                }
        }
        stochasticRelaxationRadiosityDoIncrementalRadianceIterations(scenePatches);

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
                stochasticRelaxationRadiosityDoIncrementalImportanceIterations(scenePatches);
                if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch ) {
                    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceRaysPerIteration = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays;
                }
            } else {
                stochasticRelaxationRadiosityDoRegularImportanceIteration(scenePatches);
            }
        }
        stochasticRelaxationRadiosityDoRegularRadianceIteration(scenePatches);
    }

    stochasticRelaxationRadiosityRecomputeDisplayColors(scenePatches);

    fprintf(stderr, "%s\n", stochasticRelaxationRadiosityGetStats());

    return false; // Always continue computing (never fully converged)
}

RADIANCEMETHOD GLOBAL_stochasticRaytracing_stochasticRelaxationRadiosity = {
    "StochJacobi",
    3,
    "Stochastic Jacobi Radiosity",
    monteCarloRadiosityDefaults,
    stochasticRelaxationRadiosityParseOptions,
    stochasticRelaxationRadiosityInit,
    stochasticRelaxationRadiosityGetStats,
    stochasticRelaxationRadiosityRender,
    mcrWriteVrml
};