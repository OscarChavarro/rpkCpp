/* Stochastic Relaxation Radiosity (currently only stochastic Jacobi) */

#include <cstdlib>
#include <cstdio>

#include "common/error.h"
#include "material/statistics.h"
#include "scene/scene.h"
#include "shared/render.h"
#include "raycasting/stochasticRaytracing/vrml/vrml.h"
#include "raycasting/stochasticRaytracing/mcrad.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "raycasting/stochasticRaytracing/stochjacobi.h"

static void SrrInit() {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.method = STOCHASTIC_RELAXATION_RADIOSITY_METHOD;
    monteCarloRadiosityInit();
}

static char *SrrGetStats() {
    static char stats[2000];
    char *p;
    int n;

    p = stats;
    sprintf(p, "Stochastic Relaxation Radiosity\nStatistics\n\n%n", &n);
    p += n;
    sprintf(p, "Iteration nr: %d\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration, &n);
    p += n;
    sprintf(p, "CPU time: %g secs\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, &n);
    p += n;
    /*sprintf(p, "Memory usage: %ld KBytes.\n%n", GetMemoryUsage()/1024, &n); p += n;*/
    sprintf(p, "%ld elements (%ld clusters, %ld surfaces)\n%n",
            GLOBAL_stochasticRaytracing_hierarchy.nr_elements, GLOBAL_stochasticRaytracing_hierarchy.nr_clusters, GLOBAL_stochasticRaytracing_hierarchy.nr_elements - GLOBAL_stochasticRaytracing_hierarchy.nr_clusters, &n);
    p += n;
    sprintf(p, "Radiance rays: %ld\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays, &n);
    p += n;
    sprintf(p, "Importance rays: %ld\n%n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays, &n);
    p += n;

    return stats;
}

/* randomly returns floor(x) or ceil(x) so that the expected value is equal to x */
static long RandomRound(float x) {
    long l = (long) floor(x);
    if ( drand48() < (x - (float) l)) {
        l++;
    }
    return l;
}

static void SrrRecomputeDisplayColors() {
    if ( GLOBAL_stochasticRaytracing_hierarchy.topcluster ) {
        McrForAllLeafElements(GLOBAL_stochasticRaytracing_hierarchy.topcluster, ElementComputeNewVertexColors);
        McrForAllLeafElements(GLOBAL_stochasticRaytracing_hierarchy.topcluster, ElementAdjustTVertexColors);
    } else {
        PatchListIterate(GLOBAL_scene_patches, monteCarloRadiosityPatchComputeNewColor);
    }
}

/* Computes quality factor on given leaf element (see PhD Ph.Bekaert p.152).
 * In the basic algorithms by Neumann et al. the quality factor would
 * correspond to the inverse of the elementary ray power. The quality factor
 * indicates the quality of the radiosity solution on a given leaf element.
 * The quality factor after different iterations is additive. It is used in order 
 * to properly merge the result of new iterations with the result of previous
 * iterations properly taking into account the number of rays and importance 
 * distribution. */
static double QualityFactor(ELEMENT *elem, double w) {
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
        return w * elem->imp;
    }
    return w / ElementScalarReflectance(elem);
}

static COLOR *ElementUnshotRadiance(ELEMENT *elem) {
    return elem->unshot_rad;
}

static void ElementIncrementRadiance(ELEMENT *elem, double w) {
    /* Each incremental iteration computes a different contribution to the
     * solution. The quality factor of the result remains constant. */
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.discardIncremental ) {
        elem->quality = 0.0;
        {
            static int wgiv = false;
            if ( !wgiv ) {
                logWarning("ElementIncrementRadiance", "Solution of incremental Jacobi steps receives zero quality");
            }
            wgiv = true;
        }
    } else {
        elem->quality = QualityFactor(elem, w);
    }

    stochasticRadiosityAddCoefficients(elem->rad, elem->received_rad, elem->basis);
    stochasticRadiosityCopyCoefficients(elem->unshot_rad, elem->received_rad, elem->basis);
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.setSource ) {
        /* copy direct illumination and forget selfemitted illumination */
        elem->rad[0] = elem->source_rad = elem->received_rad[0];
    }
    stochasticRadiosityClearCoefficients(elem->received_rad, elem->basis);
}

static void PrintIncrementalRadianceStats() {
    fprintf(stderr, "%g secs., radiance rays = %ld (%ld not to background), unshot flux = ",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays - GLOBAL_stochasticRaytracing_monteCarloRadiosityState.numberOfMisses);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.print(stderr);
    fprintf(stderr, ", total flux = ");
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.print(stderr);
    fprintf(stderr, ", indirect importance weighted unshot flux = ");
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux.print(stderr);
    fprintf(stderr, "\n");
}

static void DoIncrementalRadianceIterations() {
    double ref_unshot;
    long step_nr = 0;

    int weighted_sampling = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling;
    int importance_driven = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven;
    if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance ) {
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven = false;
    } /* temporarily switch it off */
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = false;

    PrintIncrementalRadianceStats();
    ref_unshot = colorSumAbsComponents(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux);
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance ) {
        ref_unshot = colorSumAbsComponents(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux);
    }
    while ( 1 ) {
        /* choose nr of rays so that power carried by each ray remains equal, and
         * proportional to the number of basis functions in the rad. approx. */
        double unshot_fraction;
        long nr_rays;
        unshot_fraction = colorSumAbsComponents(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux) / ref_unshot;
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance ) {
            unshot_fraction = colorSumAbsComponents(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux) / ref_unshot;
        }
        if ( unshot_fraction < 0.01 )
            break;    /* only 1/100th of selfemitted power remains unshot */
        nr_rays = RandomRound(unshot_fraction * (double) GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays * approxdesc[GLOBAL_stochasticRaytracing_monteCarloRadiosityState.approximationOrderType].basis_size);

        step_nr++;
        fprintf(stderr, "Incremental radiance propagation step %ld: %.3f%% unshot power left.\n",
                step_nr, 100. * unshot_fraction);

        DoStochasticJacobiIteration(nr_rays, ElementUnshotRadiance, nullptr, ElementIncrementRadiance);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.setSource = false;    /* direct illumination is copied to SOURCE_FLUX(P) only
				 * the first time. */

        monteCarloRadiosityUpdateCpuSecs();
        PrintIncrementalRadianceStats();
        if ( unshot_fraction > 0.3 ) {
            SrrRecomputeDisplayColors();
            RenderNewDisplayList();
            RenderScene();
        }
    }

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven = importance_driven;    /* switch it back on if it was on */
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = weighted_sampling;
}

static float ElementUnshotImportance(ELEMENT *elem) {
    return elem->unshot_imp;
}

static void ElementIncrementImportance(ELEMENT *elem, double w) {
    elem->imp += elem->received_imp;
    elem->unshot_imp = elem->received_imp;
    elem->received_imp = 0.;
}

static void PrintIncrementalImportanceStats() {
    fprintf(stderr, "%g secs., importance rays = %ld, unshot importance = %g, total importance = %g, total area = %g\n",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp, GLOBAL_statistics_totalArea);
}

static void DoIncrementalImportanceIterations() {
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

    PrintIncrementalRadianceStats();
    while ( 1 ) {
        /* choose nr of rays so that power carried by each ray is the same, and
         * proportional to the number of basis functions in the rad. approx. */
        double unshot_fraction = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp / GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp;
        long nr_rays = RandomRound(unshot_fraction * (double) GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialNumberOfRays);
        if ( unshot_fraction < 0.01 ) {
            break;
        }

        step_nr++;
        fprintf(stderr, "Incremental importance propagation step %ld: %.3f%% unshot importance left.\n",
                step_nr, 100. * unshot_fraction);

        DoStochasticJacobiIteration(nr_rays, nullptr, ElementUnshotImportance, ElementIncrementImportance);

        monteCarloRadiosityUpdateCpuSecs();
        PrintIncrementalImportanceStats();
    }

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.radianceDriven = radiance_driven;    /* switch on again */
    GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing = do_h_meshing;
    GLOBAL_stochasticRaytracing_hierarchy.clustering = clustering;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = weighted_sampling;
}

static COLOR *ElementRadiance(ELEMENT *elem) {
    return elem->rad;
}

static void ElementUpdateRadiance(ELEMENT *elem, double w) {
    double k = (double) GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevTracedRays / (double) (GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays > 0 ? GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays : 1);

    if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.naiveMerging ) {
        double quality = QualityFactor(elem, w);
        if ( elem->quality < EPSILON ) {
            k = 0.;    /* solution of this iteration takes over */
        } else if ( quality < EPSILON ) {
            k = 1.;    /* keep result of previous iterations */
        } else if ( elem->quality + quality > EPSILON ) {
            k = elem->quality / (elem->quality + quality);
        } else {  /* quality of new solution is so high that it must take over */
            k = 0.;
        }
        elem->quality += quality;    /* add quality */
    }

    /* subtract source radiosity */
    colorSubtract(elem->rad[0], elem->source_rad, elem->rad[0]);

    /* combine with previous results */
    stochasticRadiosityScaleCoefficients(k, elem->rad, elem->basis);
    stochasticRadiosityScaleCoefficients((1. - k), elem->received_rad, elem->basis);
    stochasticRadiosityAddCoefficients(elem->rad, elem->received_rad, elem->basis);

    /* re-add source radiosity */
    colorAdd(elem->rad[0], elem->source_rad, elem->rad[0]);

    /* clear unshot and received radiance */
    stochasticRadiosityClearCoefficients(elem->unshot_rad, elem->basis);
    stochasticRadiosityClearCoefficients(elem->received_rad, elem->basis);
}

static void PrintRegularStats() {
    fprintf(stderr, "%g secs., radiance rays = %ld (%ld not to background), unshot flux = ",
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.cpuSeconds, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays - GLOBAL_stochasticRaytracing_monteCarloRadiosityState.numberOfMisses);
    fprintf(stderr, ", total flux = ");
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.print(stderr);
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
        fprintf(stderr, "\ntotal importance rays = %ld, total importance = %g, GLOBAL_statistics_totalArea = %g",
                GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays, GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp, GLOBAL_statistics_totalArea);
    }
    fprintf(stderr, "\n");
}

static void DoRegularRadianceIteration() {
    fprintf(stderr, "Regular radiance iteration %d:\n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration);
    DoStochasticJacobiIteration(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.raysPerIteration, ElementRadiance, nullptr, ElementUpdateRadiance);

    monteCarloRadiosityUpdateCpuSecs();
    PrintRegularStats();
}

static float ElementImportance(ELEMENT *elem) {
    return elem->imp;
}

static void ElementUpdateImportance(ELEMENT *elem, double w) {
    double k = (double) GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevImportanceTracedRays / (double) GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays;

    elem->imp = k * (elem->imp - elem->source_imp) +
                (1. - k) * elem->received_imp + elem->source_imp;
    elem->unshot_imp = elem->received_imp = 0.;
}

static void DoRegularImportanceIteration() {
    long nr_rays;
    int do_h_meshing = GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing;
    CLUSTERING_MODE clustering = GLOBAL_stochasticRaytracing_hierarchy.clustering;
    int weighted_sampling = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling;
    GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing = false;
    GLOBAL_stochasticRaytracing_hierarchy.clustering = NO_CLUSTERING;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = false;

    nr_rays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceRaysPerIteration;
    fprintf(stderr, "Regular importance iteration %d:\n", GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration);
    DoStochasticJacobiIteration(nr_rays, nullptr, ElementImportance, ElementUpdateImportance);

    monteCarloRadiosityUpdateCpuSecs();
    PrintRegularStats();

    GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing = do_h_meshing;
    GLOBAL_stochasticRaytracing_hierarchy.clustering = clustering;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.weightedSampling = weighted_sampling;
}

/* Resets to zero all kind of things that should be reset to zero after a first
 * iteration of which the result only is to be used as the input of subsequent
 * iterations. Basically, everything that needs to be divided by the number of
 * rays except radiosity and importance needs to be reset to zero. This is
 * required for some of the experimental stuff to work. */
static void ElementDiscardIncremental(ELEMENT *elem) {
    elem->quality = 0.;

    /* recurse */
    McrForAllChildrenElements(elem, ElementDiscardIncremental);
}

static void DiscardIncremental() {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.prevTracedRays = 0;

    ElementDiscardIncremental(GLOBAL_stochasticRaytracing_hierarchy.topcluster);
}

static int SrrDoStep() {
    monteCarloRadiosityPreStep();

    /* do some real work now */
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.currentIteration == 1 ) {
        int initial_nr_of_rays = 0;
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.doNonDiffuseFirstShot )
            doNonDiffuseFirstShot();
        initial_nr_of_rays = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays;

        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
            if ( !GLOBAL_stochasticRaytracing_monteCarloRadiosityState.incrementalUsesImportance ) {
                logWarning(nullptr, "Importance is only used from the second iteration on ...");
            } else if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdated ) {
                GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdated = false;

                /* propagate importance changes */
                DoIncrementalImportanceIterations();
                if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch ) {
                    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceRaysPerIteration = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays;
                }
            }
        }
        DoIncrementalRadianceIterations();

        /* subsequent regular iteratoins will take as many rays as in the whole
         * sequence of incremental iteration steps. */
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.raysPerIteration = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays - initial_nr_of_rays;

        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.discardIncremental ) {
            DiscardIncremental();
        }
    } else {
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven ) {
            if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdated ) {
                GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdated = false;

                /* propagate importance changes */
                DoIncrementalImportanceIterations();
                if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceUpdatedFromScratch ) {
                    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceRaysPerIteration = GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceTracedRays;
                }
            } else {
                DoRegularImportanceIteration();
            }
        }
        DoRegularRadianceIteration();
    }

    SrrRecomputeDisplayColors();

    fprintf(stderr, "%s\n", SrrGetStats());

    return false; /* always continu computing (never fully converged) */
}

static void McrRenderPatch(PATCH *patch) {
    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.inited ) {
        McrForAllSurfaceLeafs(TOPLEVEL_ELEMENT(patch), McrRenderElement);
    } else {
        RenderPatch(patch);
    }    /* not yet initialised */
}

static void SrrRender() {
    if ( renderopts.frustum_culling ) {
        RenderWorldOctree(McrRenderPatch);
    } else PatchListIterate(GLOBAL_scene_patches, McrRenderPatch);
}

RADIANCEMETHOD StochasticRelaxationRadiosity = {
        "StochJacobi",
        3,
        "Stochastic Jacobi Radiosity",
        monteCarloRadiosityDefaults,
        stochasticRelaxationRadiosityParseOptions,
        stochasticRelaxationRadiosityPrintOptions,
        SrrInit,
        SrrDoStep,
        monteCarloRadiosityTerminate,
        monteCarloRadiosityGetRadiance,
        monteCarloRadiosityCreatePatchData,
        monteCarloRadiosityPrintPatchData,
        monteCarloRadiosityDestroyPatchData,
        SrrGetStats,
        SrrRender,
        SrrRecomputeDisplayColors,
        monteCarloRadiosityUpdateMaterial,
        mcrWriteVrml
};


