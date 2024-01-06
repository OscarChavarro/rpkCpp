/*
determination of constant control radiosity value
*/

#include "common/error.h"
#include "scene/scene.h"
#include "raycasting/stochasticRaytracing/elementmcrad.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/ccr.h"

static COLOR *(*get_radiance)(ELEMENT *);

static COLOR (*get_scaling)(ELEMENT *);

/* initial guess for constant control radiance value */
static void InitialControlRadiosity(COLOR *minRad, COLOR *maxRad, COLOR *fmin, COLOR *fmax) {
    COLOR totalflux, maxrad;
    double area = 0.;
    colorClear(totalflux);
    colorClear(maxrad);

    /* initial interval: 0 ... maxrad */
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    REC_ForAllSurfaceLeafs(elem, TOPLEVEL_ELEMENT(P))
                            {
                                COLOR rad = get_radiance(elem)[0];
                                float warea = elem->area;    /* weighted area */
                                if ( mcr.importance_driven && mcr.method != RWR ) {
                                    warea *= (elem->imp - elem->source_imp); /* multiply with received importance */
                                }
                                /* factor M_PI is omitted everywhere */
                                COLORADDSCALED(totalflux, /* M_PI* */ warea, rad, totalflux);
                                area += warea;
                                COLORMAX(maxrad, rad, maxrad);
                            }
                    REC_EndForAllSurfaceLeafs;
                }
    EndForAll;

    colorClear(*minRad);
    *fmin = totalflux;

    *maxRad = maxrad;
    COLORSCALE(area, maxrad, *fmax);
    COLORSUBTRACT(*fmax, totalflux, *fmax);
}

#define NRINTERVALS 10

static void RefineComponent(float *minRad, float *maxRad, float *fmin, float *fmax,
                            float *f, float *rad) {
    int i, imin;

    /* find subinterval containing the minimum */
    *fmax = f[0];
    *fmin = f[0], imin = 0;
    for ( i = 1; i <= NRINTERVALS; i++ ) {
        if ( f[i] < *fmin ) {
            *fmin = f[i];
            imin = i;
        }
        if ( f[i] > *fmax ) {
            *fmax = f[i];
        }
    }

    if ( imin == 0 ) {            /* first subinterval contains minimum */
        *minRad = rad[0];
        *maxRad = rad[1];
    } else if ( imin == NRINTERVALS ) {    /* last subinterval contains minimum */
        *minRad = rad[NRINTERVALS - 1];
        *maxRad = rad[NRINTERVALS];
    } else {
        if ( f[imin - 1] < f[imin + 1] ) {    /* subinterval left of imin contains minimum */
            *minRad = rad[imin - 1];
            *maxRad = rad[imin];
        } else {                /* subinterval right of imin */
            *minRad = rad[imin];
            *maxRad = rad[imin + 1];
        }
    }
}

/* Finds subinterval containing optimal constant control radiosity value. *.
 * Uses regular interval subdivision (generalisation of the bisection
 * method). Does so component wise. */
static void RefineControlRadiosity(COLOR *minRad, COLOR *maxRad, COLOR *fmin, COLOR *fmax) {
    COLOR color_one;
    COLOR f[NRINTERVALS + 1], rad[NRINTERVALS + 1], d;
    int i, s;

    COLORSETMONOCHROME(color_one, 1.);

    /* initialisations. rad[i] = radiosity at boundary i. */
    COLORSUBTRACT(*maxRad, *minRad, d);
    for ( i = 0; i <= NRINTERVALS; i++ ) {
        colorClear(f[i]);
        COLORADDSCALED(*minRad, (double) i / (double) NRINTERVALS, d, rad[i]);
    }

    /* determine value of F(beta) = sum_i (area_i * fabs(B_i - beta)) on
     * a regular subdivision of the interval */
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    REC_ForAllSurfaceLeafs(elem, TOPLEVEL_ELEMENT(P))
                            {
                                COLOR B = get_radiance(elem)[0];
                                COLOR s = get_scaling ? get_scaling(elem) : color_one;
                                float warea = elem->area;    /* weighted area */
                                if ( mcr.importance_driven && mcr.method != RWR ) {
                                    warea *= (elem->imp - elem->source_imp); /* multiply with received importance */
                                }
                                for ( i = 0; i <= NRINTERVALS; i++ ) {
                                    COLOR t;
                                    COLORPROD(s, rad[i], t);
                                    COLORSUBTRACT(B, t, t);
                                    COLORABS(t, t);
                                    COLORADDSCALED(f[i], warea, t, f[i]);
                                }
                            }
                    REC_EndForAllSurfaceLeafs;
                }
    EndForAll;

    /* find sub-interval containing optimal control radiosity (component-wise) */
    for ( s = 0; s < SPECTRUM_CHANNELS; s++ ) {
        float fc[NRINTERVALS + 1], radc[NRINTERVALS + 1];
        for ( i = 0; i <= NRINTERVALS; i++ ) {    /* copy components */
            fc[i] = f[i].spec[s];
            radc[i] = rad[i].spec[s];
        }
        RefineComponent(&(minRad->spec[s]), &(maxRad->spec[s]),
                        &(fmin->spec[s]), &(fmax->spec[s]), fc, radc);
    }
}

#undef NRINTERVALS

COLOR DetermineControlRadiosity(COLOR *(*GetRadiance)(ELEMENT *),
                                COLOR (*GetScaling)(ELEMENT *)) {
    COLOR minRad, maxRad, fmin, fmax, beta, delta, f_orig;
    float eps = 0.001;
    int sweep = 0;

    get_radiance = GetRadiance;
    get_scaling = GetScaling;
    colorClear(beta);
    if ( !get_radiance ) {
        return beta;
    }

    fprintf(stderr, "Determining optimal control radiosity value ... ");
    InitialControlRadiosity(&minRad, &maxRad, &fmin, &fmax);
    f_orig = fmin;    /* initial minRad=0, f/f_orig will determine
			 * possible efficiency gain */

    COLORSUBTRACT(fmax, fmin, delta);
    COLORADDSCALED(delta, (-eps), fmin, delta);
    while ((COLORMAXCOMPONENT(delta) > 0.) || sweep < 4 ) {
        sweep++;
        RefineControlRadiosity(&minRad, &maxRad, &fmin, &fmax);
        COLORSUBTRACT(fmax, fmin, delta);
        COLORADDSCALED(delta, (-eps), fmin, delta);
    }

    COLORADD(minRad, maxRad, beta);
    COLORSCALE(0.5, beta, beta);
    beta.print(stderr);
    fprintf(stderr, " (%g lux)", M_PI * ColorLuminance(beta));
    fprintf(stderr, "\n");
    return beta;
}
