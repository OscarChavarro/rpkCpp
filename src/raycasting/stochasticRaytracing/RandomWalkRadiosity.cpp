#include <cstdio>

#include "common/error.h"
#include "material/statistics.h"
#include "scene/scene.h"
#include "raycasting/stochasticRaytracing/mcrad.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/tracepath.h"
#include "raycasting/stochasticRaytracing/stochjacobi.h"

static void
RwrPrintPatchData(FILE *out, PATCH *patch) {
    McrPrintElement(out, TOPLEVEL_ELEMENT(patch));
}

static void
RwrInit() {
    mcr.method = RANDOM_WALK_RADIOSITY_METHOD;
    monteCarloRadiosityInit();
}

static void
PrintStats() {
    fprintf(stderr, "%g secs., total radiance rays = %ld",
            mcr.cpu_secs, mcr.traced_rays);
    fprintf(stderr, ", total flux = ");
    mcr.total_flux.print(stderr);
    if ( mcr.importance_driven ) {
        fprintf(stderr, "\ntotal importance rays = %ld, total importance = %g, GLOBAL_statistics_totalArea = %g",
                mcr.imp_traced_rays, mcr.total_ymp, GLOBAL_statistics_totalArea);
    }
    fprintf(stderr, "\n");
}

/* used as un-normalised probability for mimicking global lines */
static double
PatchArea(PATCH *P) {
    return P->area;
}

/* probability proportional to power to be propagated. */
static double
ScalarSourcePower(PATCH *P) {
    COLOR radiance = TOPLEVEL_ELEMENT(P)->source_rad;
    return /* M_PI * */ P->area * colorSumAbsComponents(radiance);
}

/* returns a double instead of a float in order to make it useful as 
 * a survival probability function */
static double
ScalarReflectance(PATCH *P) {
    return monteCarloRadiosityScalarReflectance(P);
}

static COLOR *
GetSelfEmittedRadiance(ELEMENT *elem) {
    static COLOR Ed[MAX_BASIS_SIZE];
    stochasticRadiosityClearCoefficients(Ed, elem->basis);
    Ed[0] = TOPLEVEL_ELEMENT(elem->pog.patch)->Ed; // Emittance
    return Ed;
}

/* subtracts (1-rho) * control radiosity from the source radiosity of each patch */
static void
ReduceSource() {
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    COLOR newsrcrad, rho;

                    colorSetMonochrome(newsrcrad, 1.);
                    rho = TOPLEVEL_ELEMENT(P)->Rd; // Reflectance
                    colorSubtract(newsrcrad, rho, newsrcrad);    /* 1-rho */
                    colorProduct(newsrcrad, mcr.control_radiance, newsrcrad);  /* (1-rho) * beta */
                    colorSubtract(TOPLEVEL_ELEMENT(P)->source_rad, newsrcrad, newsrcrad);    /* E - (1-rho) beta */
                    TOPLEVEL_ELEMENT(P)->source_rad = newsrcrad;
                }
    EndForAll;
}

static double
ScoreWeight(PATH *path, int n) {
    double w = 0.;
    int t = path->nrnodes - ((mcr.rw_numlast > 0) ? mcr.rw_numlast : 1);

    switch ( mcr.rw_estimator_kind ) {
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
                }       /* survival prob of n...nrnodes-2th node */
            }
            break;
        case RW_NLAST:
            if ( n == t ) {
                /* 1/absorption probability of the last path node */
                w = 1. / (1. - path->nodes[path->nrnodes - 1].probability);
            } else if ( n > t ) {
                w = 1.;
            }
            break;
        default:
            Fatal(-1, "ScoreWeight", "Unknown random walk estimator kind %d", mcr.rw_estimator_kind);
    }
    return w;
}

static void
ShootingScore(PATH *path, long nr_paths, double (*birth_prob)(PATCH *)) {
    COLOR accum_pow;
    int n;
    PATHNODE *node = &path->nodes[0];

    /* path->nodes[0].probability is birth probability of the path */
    colorScale((node->patch->area / node->probability), TOPLEVEL_ELEMENT(node->patch)->source_rad, accum_pow);
    for ( n = 1, node++; n < path->nrnodes; n++, node++ ) {
        double uin = 0., vin = 0., uout = 0., vout = 0., r = 1., w;
        int i;
        PATCH *P = node->patch;
        COLOR Rd = TOPLEVEL_ELEMENT(P)->Rd;
        colorProduct(accum_pow, Rd, accum_pow);

        PatchUniformUV(P, &node->inpoint, &uin, &vin);
        if ( !mcr.continuous_random_walk ) {
            r = 0.;
            if ( n < path->nrnodes - 1 ) {
                /* not continuous random walk and not node of absorption */
                PatchUniformUV(P, &node->outpoint, &uout, &vout);
            }
        }

        w = ScoreWeight(path, n);

        for ( i = 0; i < getTopLevelPatchBasis(P)->size; i++ ) {
            double dual = getTopLevelPatchBasis(P)->dualfunction[i](uin, vin) / P->area;
            colorAddScaled(getTopLevelPatchReceivedRad(P)[i], (w * dual / (double) nr_paths), accum_pow, getTopLevelPatchReceivedRad(P)[i]);

            if ( !mcr.continuous_random_walk ) {
                double basf = getTopLevelPatchBasis(P)->function[i](uout, vout);
                r += dual * P->area * basf;
            }
        }

        colorScale((r / node->probability), accum_pow, accum_pow);
    }
}

static void
ShootingUpdate(PATCH *P, double w) {
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
DoShootingIteration() {
    long nr_walks;

    nr_walks = mcr.initial_nr_rays;
    if ( mcr.continuous_random_walk ) {
        nr_walks *= approxdesc[mcr.approx_type].basis_size;
    } else {
        nr_walks *= pow(approxdesc[mcr.approx_type].basis_size, 1. / (1. -
                colorMaximumComponent(GLOBAL_statistics_averageReflectivity)));
    }

    fprintf(stderr, "Shooting iteration %d (%ld paths, approximately %ld rays)\n",
            mcr.iteration_nr,
            nr_walks, (long) floor((double) nr_walks / (1. -
                    colorMaximumComponent(GLOBAL_statistics_averageReflectivity))));

    TracePaths(nr_walks,
               ScalarSourcePower, ScalarReflectance,
               ShootingScore,
               ShootingUpdate);
}

/* determines control radiosity value for collision gathering estimator */
static COLOR
DetermineGatheringControlRadiosity() {
    COLOR c1, c2, cr;
    colorClear(c1);
    colorClear(c2);
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    COLOR absorb, rho, Ed, num, denom;

                    colorSetMonochrome(absorb, 1.);
                    rho = TOPLEVEL_ELEMENT(P)->Rd;
                    colorSubtract(absorb, rho, absorb);    /* 1-rho */

                    Ed = TOPLEVEL_ELEMENT(P)->source_rad;
                    colorProduct(absorb, Ed, num);
                    colorAddScaled(c1, P->area, num, c1);    /* A_P (1-rho_P) E_P */

                    colorProduct(absorb, absorb, denom);
                    colorAddScaled(c2, P->area, denom, c2);    /* A_P (1-rho_P)^2 */
                }
    EndForAll;

    colorDivide(c1, c2, cr);
    fprintf(stderr, "Control radiosity value = ");
    cr.print(stderr);
    fprintf(stderr, ", luminosity = %g\n", colorLuminance(cr));

    return cr;
}

static void
CollisionGatheringScore(PATH *path, long nr_paths, double (*birth_prob)(PATCH *)) {
    COLOR accum_rad;
    int n;
    PATHNODE *node = &path->nodes[path->nrnodes - 1];
    accum_rad = TOPLEVEL_ELEMENT(node->patch)->source_rad;
    for ( n = path->nrnodes - 2, node--; n >= 0; n--, node-- ) {
        double uin = 0., vin = 0., uout = 0., vout = 0., r = 1.;
        int i;
        PATCH *P = node->patch;
        COLOR Rd = TOPLEVEL_ELEMENT(P)->Rd;
        colorProduct(Rd, accum_rad, accum_rad);

        PatchUniformUV(P, &node->outpoint, &uout, &vout);
        if ( !mcr.continuous_random_walk ) {
            r = 0.;
            if ( n > 0 ) {
                /* not continuous random walk and not birth node */
                PatchUniformUV(P, &node->inpoint, &uin, &vin);
            }
        }

        for ( i = 0; i < getTopLevelPatchBasis(P)->size; i++ ) {
            double dual = getTopLevelPatchBasis(P)->dualfunction[i](uout, vout);    /* = dual basis f * area */
            colorAddScaled(getTopLevelPatchReceivedRad(P)[i], dual, accum_rad, getTopLevelPatchReceivedRad(P)[i]);

            if ( !mcr.continuous_random_walk ) {
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
GatheringUpdate(PATCH *P, double w) {
    /* use un-shot rad for accumulating sum of contributions */
    stochasticRadiosityAddCoefficients(getTopLevelPatchUnShotRad(P), getTopLevelPatchReceivedRad(P),
                                       getTopLevelPatchBasis(P));
    stochasticRadiosityCopyCoefficients(getTopLevelPatchRad(P), getTopLevelPatchUnShotRad(P), getTopLevelPatchBasis(P));

    /* divide by nr of samples */
    if ( TOPLEVEL_ELEMENT(P)->ng > 0 )
        stochasticRadiosityScaleCoefficients((1.0f / (float)TOPLEVEL_ELEMENT(P)->ng), getTopLevelPatchRad(P), getTopLevelPatchBasis(P));

    /* add source radiance (source term estimation suppresion!) */
    colorAdd(getTopLevelPatchRad(P)[0], TOPLEVEL_ELEMENT(P)->source_rad, getTopLevelPatchRad(P)[0]);

    if ( mcr.constant_control_variate ) {
        /* add constant control radiosity value */
        COLOR cr = mcr.control_radiance;
        if ( mcr.indirect_only ) {
            COLOR Rd = TOPLEVEL_ELEMENT(P)->Rd;
            colorProduct(Rd, mcr.control_radiance, cr);
        }
        colorAdd(getTopLevelPatchRad(P)[0], cr, getTopLevelPatchRad(P)[0]);
    }

    stochasticRadiosityClearCoefficients(getTopLevelPatchReceivedRad(P), getTopLevelPatchBasis(P));
}

static void
DoGatheringIteration() {
    long nr_walks = mcr.initial_nr_rays;
    if ( mcr.continuous_random_walk ) {
        nr_walks *= approxdesc[mcr.approx_type].basis_size;
    } else {
        nr_walks *= pow(approxdesc[mcr.approx_type].basis_size, 1. / (1. -
                colorMaximumComponent(GLOBAL_statistics_averageReflectivity)));
    }

    if ( mcr.constant_control_variate && mcr.iteration_nr == 1 ) {
        /* constant control variate for gathering random walk radiosity */
        mcr.control_radiance = DetermineGatheringControlRadiosity();
        ReduceSource();    /* do this only once!! */
    }

    fprintf(stderr, "Collision gathering iteration %d (%ld paths, approximately %ld rays)\n",
            mcr.iteration_nr,
            nr_walks, (long) floor((double) nr_walks / (1. -
                    colorMaximumComponent(GLOBAL_statistics_averageReflectivity))));

    TracePaths(nr_walks,
               PatchArea, ScalarReflectance,
               CollisionGatheringScore,
               GatheringUpdate);
}

static void
UpdateSourceIllum(ELEMENT *elem, double w) {
    stochasticRadiosityCopyCoefficients(elem->rad, elem->received_rad, elem->basis);
    elem->source_rad = elem->received_rad[0];
    stochasticRadiosityClearCoefficients(elem->unshot_rad, elem->basis);
    stochasticRadiosityClearCoefficients(elem->received_rad, elem->basis);
}

static void
DoFirstShot() {
    long nr_rays = mcr.initial_nr_rays * approxdesc[mcr.approx_type].basis_size;
    fprintf(stderr, "First shot (%ld rays):\n", nr_rays);
    DoStochasticJacobiIteration(nr_rays, GetSelfEmittedRadiance, nullptr, UpdateSourceIllum);
    PrintStats();
}

static int
RwrDoStep() {
    monteCarloRadiosityPreStep();

    if ( mcr.iteration_nr == 1 ) {
        if ( mcr.indirect_only ) {
            DoFirstShot();
        }
    }

    switch ( mcr.rw_estimator_type ) {
        case RW_SHOOTING:
            DoShootingIteration();
            break;
        case RW_GATHERING:
            DoGatheringIteration();
            break;
        default:
            Fatal(-1, "RwrDoStep", "Unknown random walk estimator type %d", mcr.rw_estimator_type);
    }

    PatchListIterate(GLOBAL_scene_patches, monteCarloRadiosityPatchComputeNewColor);
    return false; /* never converged */
}

static void
RwrTerminate() {
    /*  TerminateLightSampling(); */
    monteCarloRadiosityTerminate();
}

static char *
RwrGetStats() {
    static char stats[2000];
    char *p;
    int n;

    p = stats;
    sprintf(p, "Random Walk Radiosity\nStatistics\n\n%n", &n);
    p += n;
    sprintf(p, "Iteration nr: %d\n%n", mcr.iteration_nr, &n);
    p += n;
    sprintf(p, "CPU time: %g secs\n%n", mcr.cpu_secs, &n);
    p += n;
    /*sprintf(p, "Memory usage: %ld KBytes.\n%n", GetMemoryUsage()/1024, &n); p += n;*/
    sprintf(p, "Radiance rays: %ld\n%n", mcr.traced_rays, &n);
    p += n;
    sprintf(p, "Importance rays: %ld\n%n", mcr.imp_traced_rays, &n);
    p += n;

    return stats;
}

RADIANCEMETHOD RandomWalkRadiosity = {
        "RandomWalk",
        3,
        "Random Walk Radiosity",
        "randwalkButton",
        monteCarloRadiosityDefaults,
        randomWalkRadiosityParseOptions,
        randomWalkRadiosityPrintOptions,
        RwrInit,
        RwrDoStep,
        RwrTerminate,
        monteCarloRadiosityGetRadiance,
        monteCarloRadiosityCreatePatchData,
        RwrPrintPatchData,
        monteCarloRadiosityDestroyPatchData,
        randomWalkRadiosityCreateControlPanel,
        randomWalkRadiosityUpdateControlPanel,
        randomWalkRadiosityShowControlPanel,
        randomWalkRadiosityHideControlPanel,
        RwrGetStats,
        (void (*)()) nullptr, // use default rendering method
        monteCarloRadiosityRecomputeDisplayColors,
        monteCarloRadiosityUpdateMaterial,
        (void (*)(FILE *)) nullptr // use default VRML model saver
};
