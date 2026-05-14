/**
Determination of constant control radiosity value
*/
#include "vsdk/toolkit/java/lang/System.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/common/color/Cie.h"

#ifdef RAYTRACING_ENABLED

#include "vsdk/toolkit/java/util/ArrayList.txx"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/McradP.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Ccr.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation.h"

Ccr::GetRadianceCallback Ccr::getRadianceCallback = nullptr;
Ccr::GetScalingCallback Ccr::getScalingCallback = nullptr;

void
Ccr::initialControlRadiosityRecursive(
    const StochasticRadiosityElement *element,
    ColorRgbMutable *minRad,
    ColorRgbMutable *maxRad,
    ColorRgbMutable *fMin,
    ColorRgbMutable *fMax,
    ColorRgbMutable *totalFluxColor,
    ColorRgbMutable *maxRadColor,
    double *area)
{
    if ( element->regularSubElements == nullptr ) {
        // Trivial case
        ColorRgbMutable rad = getRadianceCallback(element)[0];
        double weightedArea = element->area;
        if ( StochasticRelaxation::activeState().importanceDriven &&
             StochasticRelaxation::activeState().method != StochasticRaytracingMethod::RANDOM_WALK_RADIOSITY_METHOD ) {
            weightedArea *= (element->importance - element->sourceImportance); // Multiply with received importance
        }
        // factor M_PI is omitted everywhere
        totalFluxColor->addScaled(*totalFluxColor, weightedArea, rad);
        *area += weightedArea;
        maxRadColor->maximum(*maxRadColor, rad);
    } else {
        // Recursive case
        for ( int i = 0; i < 4; i++ ) {
            initialControlRadiosityRecursive(
                static_cast<StochasticRadiosityElement *>(element->regularSubElements[i]),
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
void
Ccr::initialControlRadiosity(
    ColorRgbMutable *minRad,
    ColorRgbMutable *maxRad,
    ColorRgbMutable *fMin,
    ColorRgbMutable *fMax,
    const java::ArrayList<Patch *> *scenePatches)
{
    ColorRgbMutable totalFluxColor(0.0, 0.0, 0.0);
    ColorRgbMutable maxRadColor(0.0, 0.0, 0.0);
    double area = 0.0;
    totalFluxColor.clear();
    maxRadColor.clear();

    // Initial interval: 0 ... maxRadColor
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        initialControlRadiosityRecursive(
                McradP::topLevelStochasticRadiosityElement(scenePatches->get(i)),
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
    fMax->scaledCopy(area, maxRadColor);
    fMax->subtract(*fMax, totalFluxColor);
}

void
Ccr::refineComponent(
    double *minRad,
    double *maxRad,
    double *fMin,
    double *fMax,
    const double *f,
    const double *rad)
{
    int iMin;

    // Find sub-interval containing the minimum
    *fMax = f[0];
    *fMin = f[0];
    iMin = 0;
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

void
Ccr::refineControlRadiosityRecursive(
    StochasticRadiosityElement *element,
    ColorRgbMutable *colorOne,
    ColorRgbMutable rad[NUMBER_OF_INTERVALS + 1],
    ColorRgbMutable f[NUMBER_OF_INTERVALS + 1])
{
    if ( element->regularSubElements == nullptr ) {
        // Trivial case
        ColorRgbMutable B = getRadianceCallback(element)[0];
        ColorRgbMutable s = getScalingCallback ? getScalingCallback(element) : *colorOne;
        double weightedArea = element->area;
        if ( StochasticRelaxation::activeState().importanceDriven &&
             StochasticRelaxation::activeState().method !=
             StochasticRaytracingMethod::RANDOM_WALK_RADIOSITY_METHOD ) {
            weightedArea *= (element->importance - element->sourceImportance); /* multiply with received importance */
        }
        for ( int i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {
            ColorRgbMutable t(0.0, 0.0, 0.0);
            t.scalarProduct(s, rad[i]);
            t.subtract(B, t);
            t.abs();
            f[i].addScaled(f[i], weightedArea, t);
        }
    } else {
        // Recursive case
        for ( int i = 0; i < 4; i++ ) {
            refineControlRadiosityRecursive(static_cast<StochasticRadiosityElement *>(element->regularSubElements[i]), colorOne, rad, f);
        }
    }
}

/**
Finds sub-interval containing optimal constant control radiosity value
Uses regular interval subdivision (generalisation of the bisection
method). Does so component wise
*/
void
Ccr::refineControlRadiosity(
    ColorRgbMutable *minRad,
    ColorRgbMutable *maxRad,
    ColorRgbMutable *fMin,
    ColorRgbMutable *fMax,
    const java::ArrayList<Patch *> *scenePatches)
{
    ColorRgbMutable colorOne(0.0, 0.0, 0.0);
    ColorRgbMutable f[NUMBER_OF_INTERVALS + 1];
    ColorRgbMutable rad[NUMBER_OF_INTERVALS + 1];
    ColorRgbMutable d(0.0, 0.0, 0.0);

    colorOne = ColorRgbMutable(1.0, 1.0, 1.0);

    // Initialisations. rad[i] = radiosity at boundary i
    d.subtract(*maxRad, *minRad);
    for ( int i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {
        f[i].clear();
        rad[i].addScaled(*minRad, static_cast<double>(i) / static_cast<double>(NUMBER_OF_INTERVALS), d);
    }

    // Determine value of F(beta) = sum_i (area_i * java::Math::abs(B_i - beta)) on
    // a regular subdivision of the interval
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        refineControlRadiosityRecursive(
            McradP::topLevelStochasticRadiosityElement(scenePatches->get(i)),
            &colorOne,
            rad,
            f);
    }

    // Find sub-interval containing optimal control radiosity (component-wise)
    double minValues[3] = {minRad->getR(), minRad->getG(), minRad->getB()};
    double maxValues[3] = {maxRad->getR(), maxRad->getG(), maxRad->getB()};
    double fMinValues[3] = {fMin->getR(), fMin->getG(), fMin->getB()};
    double fMaxValues[3] = {fMax->getR(), fMax->getG(), fMax->getB()};

    for ( int s = 0; s < 3; s++ ) {
        double fc[NUMBER_OF_INTERVALS + 1];
        double radC[NUMBER_OF_INTERVALS + 1];
        for ( int i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {
            // Copy components
            switch ( s ) {
                case 0:
                    fc[i] = f[i].getR();
                    radC[i] = rad[i].getR();
                    break;
                case 1:
                    fc[i] = f[i].getG();
                    radC[i] = rad[i].getG();
                    break;
                case 2:
                    fc[i] = f[i].getB();
                    radC[i] = rad[i].getB();
                    break;
                default:
                    break;
            }
        }
        refineComponent(
            &minValues[s],
            &maxValues[s],
            &fMinValues[s],
            &fMaxValues[s],
            fc,
            radC);
    }

    *minRad = ColorRgbMutable(minValues[0], minValues[1], minValues[2]);
    *maxRad = ColorRgbMutable(maxValues[0], maxValues[1], maxValues[2]);
    *fMin = ColorRgbMutable(fMinValues[0], fMinValues[1], fMinValues[2]);
    *fMax = ColorRgbMutable(fMaxValues[0], fMaxValues[1], fMaxValues[2]);
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
ColorRgbMutable
Ccr::determineControlRadiosity(
    GetRadianceCallback getRadiance,
    GetScalingCallback getScaling,
    const java::ArrayList<Patch *> *scenePatches)
{
    ColorRgbMutable minRad(0.0, 0.0, 0.0);
    ColorRgbMutable maxRad(0.0, 0.0, 0.0);
    ColorRgbMutable fMin(0.0, 0.0, 0.0);
    ColorRgbMutable fMax(0.0, 0.0, 0.0);
    ColorRgbMutable beta(0.0, 0.0, 0.0);
    ColorRgbMutable delta(0.0, 0.0, 0.0);
    double eps = 0.001;
    int sweep = 0;

    getRadianceCallback = getRadiance;
    getScalingCallback = getScaling;
    beta.clear();
    if ( getRadianceCallback == nullptr ) {
        return beta;
    }

    java::System::err.printf("Determining optimal control radiosity value ... ");
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
    beta.print(&java::System::err);
    java::System::err.printf(" (%g lux)", M_PI * Cie::spectrumLuminance(beta.getR(), beta.getG(), beta.getB()));
    java::System::err.printf("\n");
    return beta;
}

#endif
