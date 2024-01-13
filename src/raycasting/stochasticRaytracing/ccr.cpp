/**
Determination of constant control radiosity value
*/

#include "common/error.h"
#include "scene/scene.h"
#include "raycasting/stochasticRaytracing/elementmcrad.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/ccr.h"

#define NUMBER_OF_INTERVALS 10

static COLOR *(*globalGetRadiance)(ELEMENT *);
static COLOR (*globalGetScaling)(ELEMENT *);

/**
Initial guess for constant control radiance value
*/
static void
initialControlRadiosity(COLOR *minRad, COLOR *maxRad, COLOR *fmin, COLOR *fmax) {
    COLOR totalFluxColor;
    COLOR maxRadColor;
    double area = 0.0;
    colorClear(totalFluxColor);
    colorClear(maxRadColor);

    // Initial interval: 0 ... maxRadColor
    for ( int i = 0; GLOBAL_scene_patches != nullptr && i < GLOBAL_scene_patches->size(); i++ ) {
        REC_ForAllSurfaceLeafs(elem, TOPLEVEL_ELEMENT(GLOBAL_scene_patches->get(i)))
                {
                    COLOR rad = globalGetRadiance(elem)[0];
                    float warea = elem->area;    /* weighted area */
                    if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven &&
                         GLOBAL_stochasticRaytracing_monteCarloRadiosityState.method != RANDOM_WALK_RADIOSITY_METHOD ) {
                        warea *= (elem->imp - elem->source_imp); /* multiply with received importance */
                    }
                    /* factor M_PI is omitted everywhere */
                    colorAddScaled(totalFluxColor, /* M_PI* */ warea, rad, totalFluxColor);
                    area += warea;
                    colorMaximum(maxRadColor, rad, maxRadColor);
                }
        REC_EndForAllSurfaceLeafs;
    }


    colorClear(*minRad);
    *fmin = totalFluxColor;

    *maxRad = maxRadColor;
    colorScale(area, maxRadColor, *fmax);
    colorSubtract(*fmax, totalFluxColor, *fmax);
}

static void
refineComponent(float *minRad, float *maxRad, float *fmin, float *fmax,
                float *f, float *rad) {
    int i, imin;

    /* find subinterval containing the minimum */
    *fmax = f[0];
    *fmin = f[0], imin = 0;
    for ( i = 1; i <= NUMBER_OF_INTERVALS; i++ ) {
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
    } else if ( imin == NUMBER_OF_INTERVALS ) {    /* last subinterval contains minimum */
        *minRad = rad[NUMBER_OF_INTERVALS - 1];
        *maxRad = rad[NUMBER_OF_INTERVALS];
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

/**
Finds sub-interval containing optimal constant control radiosity value
Uses regular interval subdivision (generalisation of the bisection
method). Does so component wise
*/
static void
refineControlRadiosity(COLOR *minRad, COLOR *maxRad, COLOR *fmin, COLOR *fmax) {
    COLOR color_one;
    COLOR f[NUMBER_OF_INTERVALS + 1], rad[NUMBER_OF_INTERVALS + 1], d;
    int i, s;

    colorSetMonochrome(color_one, 1.);

    /* initialisations. rad[i] = radiosity at boundary i. */
    colorSubtract(*maxRad, *minRad, d);
    for ( i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {
        colorClear(f[i]);
        colorAddScaled(*minRad, (double) i / (double) NUMBER_OF_INTERVALS, d, rad[i]);
    }

    // Determine value of F(beta) = sum_i (area_i * fabs(B_i - beta)) on
    // a regular subdivision of the interval
    for ( int i = 0; GLOBAL_scene_patches != nullptr && i < GLOBAL_scene_patches->size(); i++ ) {
        REC_ForAllSurfaceLeafs(elem, TOPLEVEL_ELEMENT(GLOBAL_scene_patches->get(i)))
            {
                COLOR B = globalGetRadiance(elem)[0];
                COLOR s = globalGetScaling ? globalGetScaling(elem) : color_one;
                float weightedArea = elem->area;
                if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven &&
                     GLOBAL_stochasticRaytracing_monteCarloRadiosityState.method !=
                     RANDOM_WALK_RADIOSITY_METHOD ) {
                    weightedArea *= (elem->imp - elem->source_imp); // Multiply with received importance
                }
                for ( i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {
                    COLOR t;
                    colorProduct(s, rad[i], t);
                    colorSubtract(B, t, t);
                    colorAbs(t, t);
                    colorAddScaled(f[i], weightedArea, t, f[i]);
                }
            }
        REC_EndForAllSurfaceLeafs;
    }

    /* find sub-interval containing optimal control radiosity (component-wise) */
    for ( s = 0; s < 3; s++ ) {
        float fc[NUMBER_OF_INTERVALS + 1], radc[NUMBER_OF_INTERVALS + 1];
        for ( i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {    /* copy components */
            fc[i] = f[i].spec[s];
            radc[i] = rad[i].spec[s];
        }
        refineComponent(&(minRad->spec[s]), &(maxRad->spec[s]),
                        &(fmin->spec[s]), &(fmax->spec[s]), fc, radc);
    }
}

COLOR
determineControlRadiosity(
    COLOR *(*getRadiance)(ELEMENT *),
    COLOR (*getScaling)(ELEMENT *))
{
    COLOR minRad;
    COLOR maxRad;
    COLOR fmin;
    COLOR fmax;
    COLOR beta;
    COLOR delta;
    COLOR f_orig;
    float eps = 0.001;
    int sweep = 0;

    globalGetRadiance = getRadiance;
    globalGetScaling = getScaling;
    colorClear(beta);
    if ( !globalGetRadiance ) {
        return beta;
    }

    fprintf(stderr, "Determining optimal control radiosity value ... ");
    initialControlRadiosity(&minRad, &maxRad, &fmin, &fmax);
    f_orig = fmin;    /* initial minRad=0, f/f_orig will determine
			 * possible efficiency gain */

    colorSubtract(fmax, fmin, delta);
    colorAddScaled(delta, (-eps), fmin, delta);
    while ((colorMaximumComponent(delta) > 0.) || sweep < 4 ) {
        sweep++;
        refineControlRadiosity(&minRad, &maxRad, &fmin, &fmax);
        colorSubtract(fmax, fmin, delta);
        colorAddScaled(delta, (-eps), fmin, delta);
    }

    colorAdd(minRad, maxRad, beta);
    colorScale(0.5, beta, beta);
    beta.print(stderr);
    fprintf(stderr, " (%g lux)", M_PI * colorLuminance(beta));
    fprintf(stderr, "\n");
    return beta;
}
