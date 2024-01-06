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
    PrintElement(out, TOPLEVEL_ELEMENT(patch));
}

static void
RwrInit() {
    mcr.method = RWR;
    McrInit();
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
    COLOR radiance = SOURCE_RAD(P);
    return /* M_PI * */ P->area * COLORSUMABSCOMPONENTS(radiance);
}

/* returns a double instead of a float in order to make it useful as 
 * a survival probability function */
static double
ScalarReflectance(PATCH *P) {
    return McrScalarReflectance(P);
}

static COLOR *
GetSelfEmittedRadiance(ELEMENT *elem) {
    static COLOR Ed[MAX_BASIS_SIZE];
    CLEARCOEFFICIENTS(Ed, elem->basis);
    Ed[0] = EMITTANCE(elem->pog.patch);
    return Ed;
}

/* subtracts (1-rho) * control radiosity from the source radiosity of each patch */
static void
ReduceSource() {
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    COLOR newsrcrad, rho;

                    colorSetMonochrome(newsrcrad, 1.);
                    rho = REFLECTANCE(P);
                    colorSubtract(newsrcrad, rho, newsrcrad);    /* 1-rho */
                    colorProduct(newsrcrad, mcr.control_radiance, newsrcrad);  /* (1-rho) * beta */
                    colorSubtract(SOURCE_RAD(P), newsrcrad, newsrcrad);    /* E - (1-rho) beta */
                    SOURCE_RAD(P) = newsrcrad;
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
    colorScale((node->patch->area / node->probability), SOURCE_RAD(node->patch), accum_pow);
    for ( n = 1, node++; n < path->nrnodes; n++, node++ ) {
        double uin = 0., vin = 0., uout = 0., vout = 0., r = 1., w;
        int i;
        PATCH *P = node->patch;
        COLOR Rd = REFLECTANCE(P);
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

        for ( i = 0; i < BAS(P)->size; i++ ) {
            double dual = BAS(P)->dualfunction[i](uin, vin) / P->area;
            colorAddScaled(RECEIVED_RAD(P)[i], (w * dual / (double) nr_paths), accum_pow, RECEIVED_RAD(P)[i]);

            if ( !mcr.continuous_random_walk ) {
                double basf = BAS(P)->function[i](uout, vout);
                r += dual * P->area * basf;
            }
        }

        colorScale((r / node->probability), accum_pow, accum_pow);
    }
}

static void
ShootingUpdate(PATCH *P, double w) {
    double k, old_quality;
    old_quality = QUALITY(P);
    QUALITY(P) += w;
    if ( QUALITY(P) < EPSILON ) {
        return;
    }
    k = old_quality / QUALITY(P);

    /* subtract selfemitted rad */
    colorSubtract(RAD(P)[0], SOURCE_RAD(P), RAD(P)[0]);

    /* weight with previous results */
    SCALECOEFFICIENTS(k, RAD(P), BAS(P));
    SCALECOEFFICIENTS((1. - k), RECEIVED_RAD(P), BAS(P));
    ADDCOEFFICIENTS(RAD(P), RECEIVED_RAD(P), BAS(P));

    /* re-add selfemitted rad */
    colorAdd(RAD(P)[0], SOURCE_RAD(P), RAD(P)[0]);

    /* clear unshot and received radiance */
    CLEARCOEFFICIENTS(UNSHOT_RAD(P), BAS(P));
    CLEARCOEFFICIENTS(RECEIVED_RAD(P), BAS(P));
}

static void
DoShootingIteration() {
    long nr_walks;

    nr_walks = mcr.initial_nr_rays;
    if ( mcr.continuous_random_walk ) {
        nr_walks *= approxdesc[mcr.approx_type].basis_size;
    } else {
        nr_walks *= pow(approxdesc[mcr.approx_type].basis_size, 1. / (1. - COLORMAXCOMPONENT(GLOBAL_statistics_averageReflectivity)));
    }

    fprintf(stderr, "Shooting iteration %d (%ld paths, approximately %ld rays)\n",
            mcr.iteration_nr,
            nr_walks, (long) floor((double) nr_walks / (1. - COLORMAXCOMPONENT(GLOBAL_statistics_averageReflectivity))));

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
                    rho = REFLECTANCE(P);
                    colorSubtract(absorb, rho, absorb);    /* 1-rho */

                    Ed = SOURCE_RAD(P);
                    colorProduct(absorb, Ed, num);
                    colorAddScaled(c1, P->area, num, c1);    /* A_P (1-rho_P) E_P */

                    colorProduct(absorb, absorb, denom);
                    colorAddScaled(c2, P->area, denom, c2);    /* A_P (1-rho_P)^2 */
                }
    EndForAll;

    colorDivide(c1, c2, cr);
    fprintf(stderr, "Control radiosity value = ");
    cr.print(stderr);
    fprintf(stderr, ", luminosity = %g\n", ColorLuminance(cr));

    return cr;
}

static void
CollisionGatheringScore(PATH *path, long nr_paths, double (*birth_prob)(PATCH *)) {
    COLOR accum_rad;
    int n;
    PATHNODE *node = &path->nodes[path->nrnodes - 1];
    accum_rad = SOURCE_RAD(node->patch);
    for ( n = path->nrnodes - 2, node--; n >= 0; n--, node-- ) {
        double uin = 0., vin = 0., uout = 0., vout = 0., r = 1.;
        int i;
        PATCH *P = node->patch;
        COLOR Rd = REFLECTANCE(P);
        colorProduct(Rd, accum_rad, accum_rad);

        PatchUniformUV(P, &node->outpoint, &uout, &vout);
        if ( !mcr.continuous_random_walk ) {
            r = 0.;
            if ( n > 0 ) {
                /* not continuous random walk and not birth node */
                PatchUniformUV(P, &node->inpoint, &uin, &vin);
            }
        }

        for ( i = 0; i < BAS(P)->size; i++ ) {
            double dual = BAS(P)->dualfunction[i](uout, vout);    /* = dual basis f * area */
            colorAddScaled(RECEIVED_RAD(P)[i], dual, accum_rad, RECEIVED_RAD(P)[i]);

            if ( !mcr.continuous_random_walk ) {
                double basf = BAS(P)->function[i](uin, vin);
                r += basf * dual;
            }
        }
        NG(P)++;

        colorScale((r / node->probability), accum_rad, accum_rad);
        colorAdd(accum_rad, SOURCE_RAD(P), accum_rad);
    }
}

static void
GatheringUpdate(PATCH *P, double w) {
    /* use un-shot rad for accumulating sum of contributions */
    ADDCOEFFICIENTS(UNSHOT_RAD(P), RECEIVED_RAD(P), BAS(P));
    COPYCOEFFICIENTS(RAD(P), UNSHOT_RAD(P), BAS(P));

    /* divide by nr of samples */
    if ( NG(P) > 0 ) SCALECOEFFICIENTS((1. / (double) NG(P)), RAD(P), BAS(P));

    /* add source radiance (source term estimation suppresion!) */
    colorAdd(RAD(P)[0], SOURCE_RAD(P), RAD(P)[0]);

    if ( mcr.constant_control_variate ) {
        /* add constant control radiosity value */
        COLOR cr = mcr.control_radiance;
        if ( mcr.indirect_only ) {
            COLOR Rd = REFLECTANCE(P);
            colorProduct(Rd, mcr.control_radiance, cr);
        }
        colorAdd(RAD(P)[0], cr, RAD(P)[0]);
    }

    CLEARCOEFFICIENTS(RECEIVED_RAD(P), BAS(P));
}

static void
DoGatheringIteration() {
    long nr_walks = mcr.initial_nr_rays;
    if ( mcr.continuous_random_walk ) {
        nr_walks *= approxdesc[mcr.approx_type].basis_size;
    } else {
        nr_walks *= pow(approxdesc[mcr.approx_type].basis_size, 1. / (1. - COLORMAXCOMPONENT(GLOBAL_statistics_averageReflectivity)));
    }

    if ( mcr.constant_control_variate && mcr.iteration_nr == 1 ) {
        /* constant control variate for gathering random walk radiosity */
        mcr.control_radiance = DetermineGatheringControlRadiosity();
        ReduceSource();    /* do this only once!! */
    }

    fprintf(stderr, "Collision gathering iteration %d (%ld paths, approximately %ld rays)\n",
            mcr.iteration_nr,
            nr_walks, (long) floor((double) nr_walks / (1. - COLORMAXCOMPONENT(GLOBAL_statistics_averageReflectivity))));

    TracePaths(nr_walks,
               PatchArea, ScalarReflectance,
               CollisionGatheringScore,
               GatheringUpdate);
}

static void
UpdateSourceIllum(ELEMENT *elem, double w) {
    COPYCOEFFICIENTS(elem->rad, elem->received_rad, elem->basis);
    elem->source_rad = elem->received_rad[0];
    CLEARCOEFFICIENTS(elem->unshot_rad, elem->basis);
    CLEARCOEFFICIENTS(elem->received_rad, elem->basis);
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
    McrPreStep();

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

    PatchListIterate(GLOBAL_scene_patches, McrPatchComputeNewColor);
    return false; /* never converged */
}

static void
RwrTerminate() {
    /*  TerminateLightSampling(); */
    McrTerminate();
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
        McrDefaults,
        RwrParseOptions,
        RwrPrintOptions,
        RwrInit,
        RwrDoStep,
        RwrTerminate,
        McrGetRadiance,
        McrCreatePatchData,
        RwrPrintPatchData,
        McrDestroyPatchData,
        RwrCreateControlPanel,
        RwrUpdateControlPanel,
        RwrShowControlPanel,
        RwrHideControlPanel,
        RwrGetStats,
        (void (*)()) nullptr, // use default rendering method
        McrRecomputeDisplayColors,
        McrUpdateMaterial,
        (void (*)(FILE *)) nullptr // use default VRML model saver
};
