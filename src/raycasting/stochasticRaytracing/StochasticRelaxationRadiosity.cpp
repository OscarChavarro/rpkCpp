/* Stochastic Relaxation Radiosity (currently only stochastic Jacobi) */

#include <cstdlib>
#include <cstdio>

#include "common/error.h"
#include "material/statistics.h"
#include "scene/scene.h"
#include "shared/render.h"
#include "raycasting/stochasticRaytracing/vrml.h"
#include "raycasting/stochasticRaytracing/mcrad.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "raycasting/stochasticRaytracing/stochjacobi.h"

static void SrrInit() {
    mcr.method = SRR;
    McrInit();
}

static char *SrrGetStats() {
    static char stats[2000];
    char *p;
    int n;

    p = stats;
    sprintf(p, "Stochastic Relaxation Radiosity\nStatistics\n\n%n", &n);
    p += n;
    sprintf(p, "Iteration nr: %d\n%n", mcr.iteration_nr, &n);
    p += n;
    sprintf(p, "CPU time: %g secs\n%n", mcr.cpu_secs, &n);
    p += n;
    /*sprintf(p, "Memory usage: %ld KBytes.\n%n", GetMemoryUsage()/1024, &n); p += n;*/
    sprintf(p, "%ld elements (%ld clusters, %ld surfaces)\n%n",
            hierarchy.nr_elements, hierarchy.nr_clusters, hierarchy.nr_elements - hierarchy.nr_clusters, &n);
    p += n;
    sprintf(p, "Radiance rays: %ld\n%n", mcr.traced_rays, &n);
    p += n;
    sprintf(p, "Importance rays: %ld\n%n", mcr.imp_traced_rays, &n);
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
    if ( hierarchy.topcluster ) {
        ForAllLeafElements(hierarchy.topcluster, ElementComputeNewVertexColors);
        ForAllLeafElements(hierarchy.topcluster, ElementAdjustTVertexColors);
    } else {
        PatchListIterate(GLOBAL_scene_patches, McrPatchComputeNewColor);
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
    if ( mcr.importance_driven ) {
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
    if ( mcr.discard_incremental ) {
        elem->quality = 0.0;
        {
            static int wgiv = false;
            if ( !wgiv ) {
                Warning("ElementIncrementRadiance", "Solution of incremental Jacobi steps receives zero quality");
            }
            wgiv = true;
        }
    } else {
        elem->quality = QualityFactor(elem, w);
    }

    ADDCOEFFICIENTS(elem->rad, elem->received_rad, elem->basis);
    COPYCOEFFICIENTS(elem->unshot_rad, elem->received_rad, elem->basis);
    if ( mcr.set_source ) {
        /* copy direct illumination and forget selfemitted illumination */
        elem->rad[0] = elem->source_rad = elem->received_rad[0];
    }
    CLEARCOEFFICIENTS(elem->received_rad, elem->basis);
}

static void PrintIncrementalRadianceStats() {
    fprintf(stderr, "%g secs., radiance rays = %ld (%ld not to background), unshot flux = ",
            mcr.cpu_secs, mcr.traced_rays, mcr.traced_rays - mcr.nrmisses);
    ColorPrint(stderr, mcr.unshot_flux);
    fprintf(stderr, ", total flux = ");
    ColorPrint(stderr, mcr.total_flux);
    fprintf(stderr, ", indirect importance weighted unshot flux = ");
    ColorPrint(stderr, mcr.imp_unshot_flux);
    fprintf(stderr, "\n");
}

static void DoIncrementalRadianceIterations() {
    double ref_unshot;
    long step_nr = 0;

    int weighted_sampling = mcr.weighted_sampling;
    int importance_driven = mcr.importance_driven;
    if ( !mcr.incremental_uses_importance ) {
        mcr.importance_driven = false;
    } /* temporarily switch it off */
    mcr.weighted_sampling = false;

    PrintIncrementalRadianceStats();
    ref_unshot = COLORSUMABSCOMPONENTS(mcr.unshot_flux);
    if ( mcr.incremental_uses_importance ) {
        ref_unshot = COLORSUMABSCOMPONENTS(mcr.imp_unshot_flux);
    }
    while ( 1 ) {
        /* choose nr of rays so that power carried by each ray remains equal, and
         * proportional to the number of basis functions in the rad. approx. */
        double unshot_fraction;
        long nr_rays;
        unshot_fraction = COLORSUMABSCOMPONENTS(mcr.unshot_flux) / ref_unshot;
        if ( mcr.incremental_uses_importance ) {
            unshot_fraction = COLORSUMABSCOMPONENTS(mcr.imp_unshot_flux) / ref_unshot;
        }
        if ( unshot_fraction < 0.01 )
            break;    /* only 1/100th of selfemitted power remains unshot */
        nr_rays = RandomRound(unshot_fraction * (double) mcr.initial_nr_rays * approxdesc[mcr.approx_type].basis_size);

        step_nr++;
        fprintf(stderr, "Incremental radiance propagation step %ld: %.3f%% unshot power left.\n",
                step_nr, 100. * unshot_fraction);

        DoStochasticJacobiIteration(nr_rays, ElementUnshotRadiance, nullptr, ElementIncrementRadiance);
        mcr.set_source = false;    /* direct illumination is copied to SOURCE_FLUX(P) only
				 * the first time. */

        McrUpdateCpuSecs();
        PrintIncrementalRadianceStats();
        if ( unshot_fraction > 0.3 ) {
            SrrRecomputeDisplayColors();
            RenderNewDisplayList();
            RenderScene();
        }
    }

    mcr.importance_driven = importance_driven;    /* switch it back on if it was on */
    mcr.weighted_sampling = weighted_sampling;
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
            mcr.cpu_secs, mcr.imp_traced_rays, mcr.unshot_ymp, mcr.total_ymp, GLOBAL_statistics_totalArea);
}

static void DoIncrementalImportanceIterations() {
    long step_nr = 0;
    int radiance_driven = mcr.radiance_driven;
    int do_h_meshing = hierarchy.do_h_meshing;
    CLUSTERING_MODE clustering = hierarchy.clustering;
    int weighted_sampling = mcr.weighted_sampling;

    if ( mcr.source_ymp < EPSILON ) {
        fprintf(stderr, "No source importance!!\n");
        return;
    }

    mcr.radiance_driven = false;    /* temporary switch it off */
    hierarchy.do_h_meshing = false;
    hierarchy.clustering = NO_CLUSTERING;
    mcr.weighted_sampling = false;

    PrintIncrementalRadianceStats();
    while ( 1 ) {
        /* choose nr of rays so that power carried by each ray is the same, and
         * proportional to the number of basis functions in the rad. approx. */
        double unshot_fraction = mcr.unshot_ymp / mcr.source_ymp;
        long nr_rays = RandomRound(unshot_fraction * (double) mcr.initial_nr_rays);
        if ( unshot_fraction < 0.01 ) {
            break;
        }

        step_nr++;
        fprintf(stderr, "Incremental importance propagation step %ld: %.3f%% unshot importance left.\n",
                step_nr, 100. * unshot_fraction);

        DoStochasticJacobiIteration(nr_rays, nullptr, ElementUnshotImportance, ElementIncrementImportance);

        McrUpdateCpuSecs();
        PrintIncrementalImportanceStats();
    }

    mcr.radiance_driven = radiance_driven;    /* switch on again */
    hierarchy.do_h_meshing = do_h_meshing;
    hierarchy.clustering = clustering;
    mcr.weighted_sampling = weighted_sampling;
}

static COLOR *ElementRadiance(ELEMENT *elem) {
    return elem->rad;
}

static void ElementUpdateRadiance(ELEMENT *elem, double w) {
    double k = (double) mcr.prev_traced_rays / (double) (mcr.traced_rays > 0 ? mcr.traced_rays : 1);

    if ( !mcr.naive_merging ) {
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
    COLORSUBTRACT(elem->rad[0], elem->source_rad, elem->rad[0]);

    /* combine with previous results */
    SCALECOEFFICIENTS(k, elem->rad, elem->basis);
    SCALECOEFFICIENTS((1. - k), elem->received_rad, elem->basis);
    ADDCOEFFICIENTS(elem->rad, elem->received_rad, elem->basis);

    /* re-add source radiosity */
    COLORADD(elem->rad[0], elem->source_rad, elem->rad[0]);

    /* clear unshot and received radiance */
    CLEARCOEFFICIENTS(elem->unshot_rad, elem->basis);
    CLEARCOEFFICIENTS(elem->received_rad, elem->basis);
}

static void PrintRegularStats() {
    fprintf(stderr, "%g secs., radiance rays = %ld (%ld not to background), unshot flux = ",
            mcr.cpu_secs, mcr.traced_rays, mcr.traced_rays - mcr.nrmisses);
    fprintf(stderr, ", total flux = ");
    ColorPrint(stderr, mcr.total_flux);
    if ( mcr.importance_driven ) {
        fprintf(stderr, "\ntotal importance rays = %ld, total importance = %g, GLOBAL_statistics_totalArea = %g",
                mcr.imp_traced_rays, mcr.total_ymp, GLOBAL_statistics_totalArea);
    }
    fprintf(stderr, "\n");
}

static void DoRegularRadianceIteration() {
    fprintf(stderr, "Regular radiance iteration %d:\n", mcr.iteration_nr);
    DoStochasticJacobiIteration(mcr.rays_per_iteration, ElementRadiance, nullptr, ElementUpdateRadiance);

    McrUpdateCpuSecs();
    PrintRegularStats();
}

static float ElementImportance(ELEMENT *elem) {
    return elem->imp;
}

static void ElementUpdateImportance(ELEMENT *elem, double w) {
    double k = (double) mcr.prev_imp_traced_rays / (double) mcr.imp_traced_rays;

    elem->imp = k * (elem->imp - elem->source_imp) +
                (1. - k) * elem->received_imp + elem->source_imp;
    elem->unshot_imp = elem->received_imp = 0.;
}

static void DoRegularImportanceIteration() {
    long nr_rays;
    int do_h_meshing = hierarchy.do_h_meshing;
    CLUSTERING_MODE clustering = hierarchy.clustering;
    int weighted_sampling = mcr.weighted_sampling;
    hierarchy.do_h_meshing = false;
    hierarchy.clustering = NO_CLUSTERING;
    mcr.weighted_sampling = false;

    nr_rays = mcr.imp_rays_per_iteration;
    fprintf(stderr, "Regular importance iteration %d:\n", mcr.iteration_nr);
    DoStochasticJacobiIteration(nr_rays, nullptr, ElementImportance, ElementUpdateImportance);

    McrUpdateCpuSecs();
    PrintRegularStats();

    hierarchy.do_h_meshing = do_h_meshing;
    hierarchy.clustering = clustering;
    mcr.weighted_sampling = weighted_sampling;
}

/* Resets to zero all kind of things that should be reset to zero after a first
 * iteration of which the result only is to be used as the input of subsequent
 * iterations. Basically, everything that needs to be divided by the number of
 * rays except radiosity and importance needs to be reset to zero. This is
 * required for some of the experimental stuff to work. */
static void ElementDiscardIncremental(ELEMENT *elem) {
    elem->quality = 0.;

    /* recurse */
    ForAllChildrenElements(elem, ElementDiscardIncremental);
}

static void DiscardIncremental() {
    mcr.nr_weighted_rays = mcr.old_nr_weighted_rays = 0;
    mcr.traced_rays = mcr.prev_traced_rays = 0;

    ElementDiscardIncremental(hierarchy.topcluster);
}

static int SrrDoStep() {
    McrPreStep();

    /* do some real work now */
    if ( mcr.iteration_nr == 1 ) {
        int initial_nr_of_rays = 0;
        if ( mcr.do_nondiffuse_first_shot )
            DoNonDiffuseFirstShot();
        initial_nr_of_rays = mcr.traced_rays;

        if ( mcr.importance_driven ) {
            if ( !mcr.incremental_uses_importance ) {
                Warning(nullptr, "Importance is only used from the second iteration on ...");
            } else if ( mcr.importance_updated ) {
                mcr.importance_updated = false;

                /* propagate importance changes */
                DoIncrementalImportanceIterations();
                if ( mcr.importance_updated_from_scratch ) {
                    mcr.imp_rays_per_iteration = mcr.imp_traced_rays;
                }
            }
        }
        DoIncrementalRadianceIterations();

        /* subsequent regular iteratoins will take as many rays as in the whole
         * sequence of incremental iteration steps. */
        mcr.rays_per_iteration = mcr.traced_rays - initial_nr_of_rays;

        if ( mcr.discard_incremental ) {
            DiscardIncremental();
        }
    } else {
        if ( mcr.importance_driven ) {
            if ( mcr.importance_updated ) {
                mcr.importance_updated = false;

                /* propagate importance changes */
                DoIncrementalImportanceIterations();
                if ( mcr.importance_updated_from_scratch ) {
                    mcr.imp_rays_per_iteration = mcr.imp_traced_rays;
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
    if ( mcr.inited ) {
        McrForAllSurfaceLeafs(TOPLEVEL_ELEMENT(patch), RenderElement);
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
        "StochJacobi", 3,
        "Stochastic Jacobi Radiosity",
        "stochrelaxButton",
        McrDefaults,
        SrrParseOptions,
        SrrPrintOptions,
        SrrInit,
        SrrDoStep,
        McrTerminate,
        McrGetRadiance,
        McrCreatePatchData,
        McrPrintPatchData,
        McrDestroyPatchData,
        SrrCreateControlPanel,
        SrrUpdateControlPanel,
        SrrShowControlPanel,
        SrrHideControlPanel,
        SrrGetStats,
        SrrRender,
        SrrRecomputeDisplayColors,
        McrUpdateMaterial,
        McrWriteVRML
};


