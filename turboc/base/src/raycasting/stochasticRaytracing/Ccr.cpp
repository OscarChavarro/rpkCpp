/**
Determination of constant control radiosity value
*/
#include "java/lang/System.h"
#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "java/util/ArrayList.txx"
#include "common/color/Cie.h"
#include "raycasting/stochasticRaytracing/McradP.h"
#include "raycasting/stochasticRaytracing/Ccr.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

Ccr::GetRadianceCallback Ccr::getRadianceCallback = NULL;
Ccr::GetScalingCallback Ccr::getScalingCallback = NULL;

void
Ccr::initCtrlRadRec(
    const StochasticRadiosityElement *element,
    ColorRgb *minRad,
    ColorRgb *maxRad,
    ColorRgb *fMin,
    ColorRgb *fMax,
    ColorRgb *totalFluxColor,
    ColorRgb *maxRadColor,
    double *area)
{
    if ( element->regularSubElements == NULL ) {
        // Trivial case
        ColorRgb rad = ColorRgb(getRadianceCallback(element)[0]);
        float weightedArea = element->area;
        if ( StochasticRelaxation::activeState().importanceDriven &&
             StochasticRelaxation::activeState().method != RANDOM_WALK_RADIOSITY_METHOD ) {
            weightedArea *= (element->importance - element->sourceImportance); // Multiply with received importance
        }
        // factor M_PI is omitted everywhere
        *totalFluxColor = ColorRgb(
            totalFluxColor->getR() + weightedArea * rad.getR(),
            totalFluxColor->getG() + weightedArea * rad.getG(),
            totalFluxColor->getB() + weightedArea * rad.getB());
        *area += weightedArea;
        maxRadColor->getR() > rad.getR() ?
            *maxRadColor = ColorRgb(maxRadColor->getR(), maxRadColor->getG(), maxRadColor->getB()) :
            *maxRadColor = ColorRgb(rad.getR(), maxRadColor->getG(), maxRadColor->getB());
        maxRadColor->getG() > rad.getG() ?
            *maxRadColor = ColorRgb(maxRadColor->getR(), maxRadColor->getG(), maxRadColor->getB()) :
            *maxRadColor = ColorRgb(maxRadColor->getR(), rad.getG(), maxRadColor->getB());
        maxRadColor->getB() > rad.getB() ?
            *maxRadColor = ColorRgb(maxRadColor->getR(), maxRadColor->getG(), maxRadColor->getB()) :
            *maxRadColor = ColorRgb(maxRadColor->getR(), maxRadColor->getG(), rad.getB());
    } else {
        // Recursive case
        for ( int i = 0; i < 4; i++ ) {
            initCtrlRadRec(
                ((StochasticRadiosityElement *)(element->regularSubElements[i])),
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
    ColorRgb *minRad,
    ColorRgb *maxRad,
    ColorRgb *fMin,
    ColorRgb *fMax,
    const ArrayList<Patch *> *scenePatches)
{
    ColorRgb totalFluxColor;
    ColorRgb maxRadColor;
    double area = 0.0;
    totalFluxColor = ColorRgb(0.0f, 0.0f, 0.0f);
    maxRadColor = ColorRgb(0.0f, 0.0f, 0.0f);

    // Initial interval: 0 ... maxRadColor
    for ( int i = 0; scenePatches != NULL && i < scenePatches->size(); i++ ) {
        initCtrlRadRec(
                McradP::topLvlStochRadElem(scenePatches->get(i)),
                minRad,
                maxRad,
                fMin,
                fMax,
                &totalFluxColor,
                &maxRadColor,
                &area);
    }

    *minRad = ColorRgb(0.0f, 0.0f, 0.0f);
    *fMin = totalFluxColor;

    *maxRad = maxRadColor;
    *fMax = ColorRgb(
        ((float)(area)) * maxRadColor.getR() - totalFluxColor.getR(),
        ((float)(area)) * maxRadColor.getG() - totalFluxColor.getG(),
        ((float)(area)) * maxRadColor.getB() - totalFluxColor.getB());
}

void
Ccr::refineComponent(
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
    ColorRgb *colorOne,
    ColorRgb rad[NUMBER_OF_INTERVALS + 1],
    ColorRgb f[NUMBER_OF_INTERVALS + 1])
{
    if ( element->regularSubElements == NULL ) {
        // Trivial case
        ColorRgb B = ColorRgb(getRadianceCallback(element)[0]);
        ColorRgb s = getScalingCallback ? getScalingCallback(element) : *colorOne;
        float weightedArea = element->area;
        if ( StochasticRelaxation::activeState().importanceDriven &&
             StochasticRelaxation::activeState().method !=
             RANDOM_WALK_RADIOSITY_METHOD ) {
            weightedArea *= (element->importance - element->sourceImportance); /* multiply with received importance */
        }
        for ( int i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {
            ColorRgb t;
            t = ColorRgb(
                Math::abs(B.getR() - s.getR() * rad[i].getR()),
                Math::abs(B.getG() - s.getG() * rad[i].getG()),
                Math::abs(B.getB() - s.getB() * rad[i].getB()));
            f[i] = ColorRgb(
                f[i].getR() + weightedArea * t.getR(),
                f[i].getG() + weightedArea * t.getG(),
                f[i].getB() + weightedArea * t.getB());
        }
    } else {
        // Recursive case
        for ( int i = 0; i < 4; i++ ) {
            refineControlRadiosityRecursive(((StochasticRadiosityElement *)(element->regularSubElements[i])), colorOne, rad, f);
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
    ColorRgb *minRad,
    ColorRgb *maxRad,
    ColorRgb *fMin,
    ColorRgb *fMax,
    const ArrayList<Patch *> *scenePatches)
{
    ColorRgb colorOne;
    ColorRgb f[NUMBER_OF_INTERVALS + 1];
    ColorRgb rad[NUMBER_OF_INTERVALS + 1];
    ColorRgb d;

    colorOne = ColorRgb(1.0f, 1.0f, 1.0f);

    // Initialisations. rad[i] = radiosity at boundary i
    d = ColorRgb(
        maxRad->getR() - minRad->getR(),
        maxRad->getG() - minRad->getG(),
        maxRad->getB() - minRad->getB());
    for ( int i = 0; i <= NUMBER_OF_INTERVALS; i++ ) {
        float a = ((float)(i)) / ((float)(NUMBER_OF_INTERVALS));
        f[i] = ColorRgb(0.0f, 0.0f, 0.0f);
        rad[i] = ColorRgb(
            minRad->getR() + a * d.getR(),
            minRad->getG() + a * d.getG(),
            minRad->getB() + a * d.getB());
    }

    // Determine value of F(beta) = sum_i (area_i * Math::abs(B_i - beta)) on
    // a regular subdivision of the interval
    for ( int i = 0; scenePatches != NULL && i < scenePatches->size(); i++ ) {
        refineControlRadiosityRecursive(
            McradP::topLvlStochRadElem(scenePatches->get(i)),
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
        float minC;
        float maxC;
        float fMinC;
        float fMaxC;
        if ( s == 0 ) {
            minC = minRad->getR();
            maxC = maxRad->getR();
            fMinC = fMin->getR();
            fMaxC = fMax->getR();
            refineComponent(&minC, &maxC, &fMinC, &fMaxC, fc, radC);
            *minRad = ColorRgb(minC, minRad->getG(), minRad->getB());
            *maxRad = ColorRgb(maxC, maxRad->getG(), maxRad->getB());
            *fMin = ColorRgb(fMinC, fMin->getG(), fMin->getB());
            *fMax = ColorRgb(fMaxC, fMax->getG(), fMax->getB());
        } else if ( s == 1 ) {
            minC = minRad->getG();
            maxC = maxRad->getG();
            fMinC = fMin->getG();
            fMaxC = fMax->getG();
            refineComponent(&minC, &maxC, &fMinC, &fMaxC, fc, radC);
            *minRad = ColorRgb(minRad->getR(), minC, minRad->getB());
            *maxRad = ColorRgb(maxRad->getR(), maxC, maxRad->getB());
            *fMin = ColorRgb(fMin->getR(), fMinC, fMin->getB());
            *fMax = ColorRgb(fMax->getR(), fMaxC, fMax->getB());
        } else {
            minC = minRad->getB();
            maxC = maxRad->getB();
            fMinC = fMin->getB();
            fMaxC = fMax->getB();
            refineComponent(&minC, &maxC, &fMinC, &fMaxC, fc, radC);
            *minRad = ColorRgb(minRad->getR(), minRad->getG(), minC);
            *maxRad = ColorRgb(maxRad->getR(), maxRad->getG(), maxC);
            *fMin = ColorRgb(fMin->getR(), fMin->getG(), fMinC);
            *fMax = ColorRgb(fMax->getR(), fMax->getG(), fMaxC);
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
multiplied with the radiance of the element. If getScaling is a NULL
pointer, no scaling is applied. Scaling is used in the context of
random walk radiosity
*/
ColorRgb
Ccr::determineControlRadiosity(
    GetRadianceCallback getRadiance,
    GetScalingCallback getScaling,
    const ArrayList<Patch *> *scenePatches)
{
    ColorRgb minRad;
    ColorRgb maxRad;
    ColorRgb fMin;
    ColorRgb fMax;
    ColorRgb beta;
    ColorRgb delta;
    float eps = 0.001f;
    int sweep = 0;

    getRadianceCallback = getRadiance;
    getScalingCallback = getScaling;
    beta = ColorRgb(0.0f, 0.0f, 0.0f);
    if ( getRadianceCallback == NULL ) {
        return beta;
    }

    System::err.printf("Determining optimal control radiosity value ... ");
    initialControlRadiosity(&minRad, &maxRad, &fMin, &fMax, scenePatches);

    delta = ColorRgb(
        fMax.getR() - (1.0f + eps) * fMin.getR(),
        fMax.getG() - (1.0f + eps) * fMin.getG(),
        fMax.getB() - (1.0f + eps) * fMin.getB());
    while ( (Math::max(delta.getR(), Math::max(delta.getG(), delta.getB())) > 0.0f) || sweep < 4 ) {
        sweep++;
        refineControlRadiosity(&minRad, &maxRad, &fMin, &fMax, scenePatches);
        delta = ColorRgb(
            fMax.getR() - (1.0f + eps) * fMin.getR(),
            fMax.getG() - (1.0f + eps) * fMin.getG(),
            fMax.getB() - (1.0f + eps) * fMin.getB());
    }

    beta = ColorRgb(
        0.5f * (minRad.getR() + maxRad.getR()),
        0.5f * (minRad.getG() + maxRad.getG()),
        0.5f * (minRad.getB() + maxRad.getB()));
    System::err.printf("(%g, %g, %g)", beta.getR(), beta.getG(), beta.getB());
    System::err.printf(" (%g lux)", M_PI * Cie::spectrumLuminance(beta.getR(), beta.getG(), beta.getB()));
    System::err.printf("\n");
    return beta;
}

#endif
