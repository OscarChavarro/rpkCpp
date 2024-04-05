/**
Determination of constant control radiosity value
*/

#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/ccr.h"

#define NUMBER_OF_INTERVALS 10

static ColorRgb *(*globalGetRadiance)(StochasticRadiosityElement *);
static ColorRgb (*globalGetScaling)(StochasticRadiosityElement *);

static void
initialControlRadiosityRecursive(
    StochasticRadiosityElement *element,
    ColorRgb *minRad,
    ColorRgb *maxRad,
    ColorRgb *fMin,
    ColorRgb *fMax,
    ColorRgb *totalFluxColor,
    ColorRgb *maxRadColor,
    double *area)
{
    if ( element->regularSubElements == nullptr ) {
        // Trivial case
        ColorRgb rad = globalGetRadiance(element)[0];
        float weightedArea = element->area;
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven &&
             GLOBAL_stochasticRaytracing_monteCarloRadiosityState.method != RANDOM_WALK_RADIOSITY_METHOD ) {
            weightedArea *= (element->importance - element->sourceImportance); // Multiply with received importance
        }
        // factor M_PI is omitted everywhere
        totalFluxColor->addScaled(*totalFluxColor, /* M_PI* */ weightedArea, rad);
        *area += weightedArea;
        colorMaximum(*maxRadColor, rad, *maxRadColor);
    } else {
        // Recursive case
        for ( int i = 0; i < 4; i++ ) {
            initialControlRadiosityRecursive(
                (StochasticRadiosityElement *)element->regularSubElements[i],
                minRad,
                maxRad,
                fMin,
                fMax,
                totalFluxColor,
                maxRadColor,
                area);
        }
    }
}

/**
Initial guess for constant control radiance value
*/
static void
initialControlRadiosity(
    ColorRgb *minRad,
    ColorRgb *maxRad,
    ColorRgb *fMin,
    ColorRgb *fMax,
    java::ArrayList<Patch *> *scenePatches)
{
    ColorRgb totalFluxColor;
    ColorRgb maxRadColor;
    double area = 0.0;
    totalFluxColor.clear();
    maxRadColor.clear();

    // Initial interval: 0 ... maxRadColor
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        initialControlRadiosityRecursive(
                topLevelGalerkinElement(scenePatches->get(i)),
                minRad,
                maxRad,
                fMin,
                fMax,
                &totalFluxColor,
                &maxRadColor,
                &area);
    }

    minRad->clear();
    *fMin = totalFluxColor;

    *maxRad = maxRadColor;
    fMax->scaledCopy((float) area, maxRadColor);
    fMax->subtract(*fMax, totalFluxColor);
}

static void
refineComponent(
    float *minRad,
    float *maxRad,
    float *fMin,
    float *fMax,
    const float *f,
    const float *rad)
{
    int iMin;

    // Find sub-interval containing the minimum
    *fMax = f[0];
    *fMin = f[0], iMin = 0;
    for ( int i = 1; i <= NUMBER_OF_INTERVALS; i++ ) {
        if ( f[i] < *fMin ) {
            *fMin = f[i];
            iMin = i;
        }
        if ( f[i] > *fMax ) {
            *fMax = f[i];
        }
    }

    if ( iMin == 0 ) {
        // First sub-interval contains minimum
        *minRad = rad[0];
        *maxRad = rad[1];
    } else if ( iMin == NUMBER_OF_INTERVALS ) {
        // Last sub-interval contains minimum
        *minRad = rad[NUMBER_OF_INTERVALS - 1];
        *maxRad = rad[NUMBER_OF_INTERVALS];
    } else {
        if ( f[iMin - 1] < f[iMin + 1] ) {
            // Sub-interval left of iMin contains minimum
            *minRad = rad[iMin - 1];
            *maxRad = rad[iMin];
        } else {
            // Sub-interval right of iMin
            *minRad = rad[iMin];
            *maxRad = rad[iMin + 1];
        }
    }
}

static void
refineControlRadiosityRecursive(
    StochasticRadiosityElement *element,
    ColorRgb *colorOne,
    ColorRgb rad[NUMBER_OF_INTERVALS + 1],
    ColorRgb f[NUMBER_OF_INTERVALS + 1])
{
    if ( element->regularSubElements == nullptr ) {
        // Trivial case
        ColorRgb B = globalGetRadiance(element)[0];
        ColorRgb s = globalGetScaling ? globalGetScaling(element) : *colorOne;
        float weightedArea = element->area;
        if ( GLOBAL_stochasticRaytracing_monteCarloRadiosityState.importanceDriven &&
             GLOBAL_stochasticRaytracing_monteCarloRadiosityState.method !=
             RANDOM_WALK_RADIOSITY_METHOD ) {
            weightedArea *= (element->importance - element->sourceImportance); /* multiply with received importance */
        }
        for ( int i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {
            ColorRgb t;
            t.scalarProduct(s, rad[i]);
            t.subtract(B, t);
            colorAbs(t, t);
            f[i].addScaled(f[i], weightedArea, t);
        }
    } else {
        // Recursive case
        for ( int i = 0; i < 4; i++ ) {
            refineControlRadiosityRecursive((StochasticRadiosityElement *)element->regularSubElements[i], colorOne, rad, f);
        }
    }
}

/**
Finds sub-interval containing optimal constant control radiosity value
Uses regular interval subdivision (generalisation of the bisection
method). Does so component wise
*/
static void
refineControlRadiosity(
    ColorRgb *minRad,
    ColorRgb *maxRad,
    ColorRgb *fMin,
    ColorRgb *fMax,
    java::ArrayList<Patch *> *scenePatches)
{
    ColorRgb colorOne;
    ColorRgb f[NUMBER_OF_INTERVALS + 1];
    ColorRgb rad[NUMBER_OF_INTERVALS + 1];
    ColorRgb d;

    colorOne.setMonochrome(1.0);

    // Initialisations. rad[i] = radiosity at boundary i
    d.subtract(*maxRad, *minRad);
    for ( int i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {
        f[i].clear();
        rad[i].addScaled(*minRad, (float) i / (float) NUMBER_OF_INTERVALS, d);
    }

    // Determine value of F(beta) = sum_i (area_i * std::fabs(B_i - beta)) on
    // a regular subdivision of the interval
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        refineControlRadiosityRecursive(
            topLevelGalerkinElement(scenePatches->get(i)),
            &colorOne,
            rad,
            f);
    }

    // Find sub-interval containing optimal control radiosity (component-wise)
    for ( int s = 0; s < 3; s++ ) {
        float fc[NUMBER_OF_INTERVALS + 1];
        float radC[NUMBER_OF_INTERVALS + 1];
        for ( int i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {
            // Copy components
            switch ( s ) {
                case 0:
                    fc[i] = f[i].spectrum[0];
                    radC[i] = rad[i].spectrum[0];
                    break;
                case 1:
                    fc[i] = f[i].spectrum[1];
                    radC[i] = rad[i].spectrum[1];
                    break;
                case 2:
                    fc[i] = f[i].spectrum[2];
                    radC[i] = rad[i].spectrum[2];
                    break;
            }
        }
        switch ( s ) {
            case 0:
                refineComponent(
                    &(minRad->spectrum[0]),
                    &(maxRad->spectrum[0]),
                    &(fMin->spectrum[0]),
                    &(fMax->spectrum[0]),
                    fc,
                    radC);
                break;
            case 1:
                refineComponent(
                    &(minRad->spectrum[1]),
                    &(maxRad->spectrum[1]),
                    &(fMin->spectrum[1]),
                    &(fMax->spectrum[1]),
                    fc,
                    radC);
                break;
            case 2:
                refineComponent(
                    &(minRad->spectrum[2]),
                    &(maxRad->spectrum[2]),
                    &(fMin->spectrum[2]),
                    &(fMax->spectrum[2]),
                    fc,
                    radC);
                break;
        }
    }
}

/**
Determines and returns optimal constant control radiosity value for
the given radiance distribution: this is, the value of beta that
minimises F(beta) = sum over all patches P of P->area times
absolute value of (globalGetRadiance(P) - globalGetScaling(P) * beta).

- getRadiance() returns the radiance to be propagated from a
given ELEMENT.
- getScaling() returns a scale factor (per color component) to be
multiplied with the radiance of the element. If getScaling is a nullptr
pointer, no scaling is applied. Scaling is used in the context of
random walk radiosity
*/
ColorRgb
determineControlRadiosity(
    ColorRgb *(*getRadiance)(StochasticRadiosityElement *),
    ColorRgb (*getScaling)(StochasticRadiosityElement *),
    java::ArrayList<Patch *> *scenePatches)
{
    ColorRgb minRad;
    ColorRgb maxRad;
    ColorRgb fMin;
    ColorRgb fMax;
    ColorRgb beta;
    ColorRgb delta;
    float eps = 0.001f;
    int sweep = 0;

    globalGetRadiance = getRadiance;
    globalGetScaling = getScaling;
    beta.clear();
    if ( globalGetRadiance == nullptr ) {
        return beta;
    }

    fprintf(stderr, "Determining optimal control radiosity value ... ");
    initialControlRadiosity(&minRad, &maxRad, &fMin, &fMax, scenePatches);

    delta.subtract(fMax, fMin);
    delta.addScaled(delta, (-eps), fMin);
    while ( (delta.maximumComponent() > 0.0) || sweep < 4 ) {
        sweep++;
        refineControlRadiosity(&minRad, &maxRad, &fMin, &fMax, scenePatches);
        delta.subtract(fMax, fMin);
        delta.addScaled(delta, (-eps), fMin);
    }

    beta.add(minRad, maxRad);
    beta.scale(0.5);
    beta.print(stderr);
    fprintf(stderr, " (%g lux)", M_PI * colorLuminance(beta));
    fprintf(stderr, "\n");
    return beta;
}
