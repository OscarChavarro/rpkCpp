/**
Estimate static adaptation for tone mapping
*/

#include <stdlib.h>

#include "java/lang/Math.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/logging/LoggerFormat.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"
#include "vsdk/toolkit/numericalAnalysis/PatchVisitor.h"
#include "app/Adaptation.h"
#include "app/LuminanceArea.h"

int Adaptation::numEntries = 0;
double Adaptation::logAreaLum = 0.0;
LuminanceArea *Adaptation::lumArea = NULL;
int Adaptation::lumAreaIndex = 0;
double Adaptation::lumMin = Double::MAX_VALUE; // Note Double::MAX_VALUE; will cause an issue here
double Adaptation::lumMax = 0.0;
Adaptation::PatchRadianceEstimateFn Adaptation::patchRadianceEstimate = NULL;

/**
A-priori estimate of a patch's radiance
*/
ColorRgb
Adaptation::initRadianceEstimate(Patch *patch) {
    ColorRgb emittance = PatchVisitor::averageEmittance(patch, XxdfComponentFlagInfo::ALL_COMPONENTS);
    ColorRgb reflectance = PatchVisitor::averageNormalAlbedo(patch, BsdfComponentInfo::BSDF_ALL_COMPONENTS);
    ColorRgb avgRad = Statistics::instance().radiance.estimatedAverageRadiance;
    return ColorRgb(
        reflectance.getR() * avgRad.getR() + (1.0 / ((double)(M_PI))) * emittance.getR(),
        reflectance.getG() * avgRad.getG() + (1.0 / ((double)(M_PI))) * emittance.getG(),
        reflectance.getB() * avgRad.getB() + (1.0 / ((double)(M_PI))) * emittance.getB());
}

int
Adaptation::adaptationLumAreaComp(const void *la1, const void *la2) {
    double l1 = ((const LuminanceArea *)(la1))->luminance;
    double l2 = ((const LuminanceArea *)(la2))->luminance;

    if ( l1 > l2 ) {
        return 1;
    }

    return l1 == l2 ? 0 : -1;
}

double
Adaptation::patchBrightnessEstimate(Patch *patch, const Cie &colorSpace) {
    ColorRgb radiance = patchRadianceEstimate(patch);
    double brightness = colorSpace.spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB());
    if ( brightness < Numeric::EPSILON ) {
        brightness = Numeric::EPSILON;
    }
    return brightness;
}

void
Adaptation::patchComputeLogAreaLum(Patch *patch, const Cie &colorSpace) {
    double brightness = Adaptation::patchBrightnessEstimate(patch, colorSpace);
    // Equation [TUMB1999b](7): log(Lwa) as mean(log(Lw)), here area-weighted over patches
    logAreaLum += patch->area * Math::log(brightness);
}

void
Adaptation::patchFillLumArea(Patch *patch, const Cie &colorSpace) {
    double brightness = Adaptation::patchBrightnessEstimate(patch, colorSpace);

    LuminanceArea &entry = lumArea[lumAreaIndex];
    entry.luminance = brightness;
    entry.area = patch->area;

    lumMin = Math::min(lumMin, entry.luminance);
    lumMax = Math::max(lumMax, entry.luminance);

    lumAreaIndex++;
    numEntries++;
}

/**
Computes the static adaptation luminance value choosing the median value
of area-weighted luminance values. Needs a correct
Statistics::instance().radiance.totalArea.
*/
double
Adaptation::meanAreaWeightedLuminance(LuminanceArea *pairs, int numPairs) {
    if ( numPairs <= 0 ) {
        return 0.0;
    }

    double areaMax = Statistics::instance().radiance.totalArea / 2.0;
    double areaCnt = 0.0;
    int pairIndex = 0;

    qsort(pairs, numPairs, sizeof(LuminanceArea), Adaptation::adaptationLumAreaComp);

    while ( pairIndex < numPairs && areaCnt < areaMax ) {
        areaCnt += pairs[pairIndex].area;
        pairIndex++;
    }

    if ( pairIndex == 0 ) {
        return pairs[0].luminance;
    }
    return pairs[pairIndex - 1].luminance;
}

/**
Estimates adaptation luminance in the current scene using the current
adaption estimation method in toneMapOptions.staticAdaptationMethod
'patch_radiance' is a pointer to a routine that computes the radiance
emitted by a patch. The result is filled in toneMapOptions.realWorldAdaptionLuminance
*/
void
Adaptation::estimateSceneAdaptation(
    PatchRadianceEstimateFn patchRadiance,
    const ArrayList<Patch *> *scenePatches,
    ToneMappingContext &toneMapOptions)
{
    patchRadianceEstimate = patchRadiance;

    switch ( toneMapOptions.staticAdaptationMethod ) {
        case TMA_NONE:
            break;
        case TMA_AVERAGE: {
            // Gibson's static adaptation after [TUMB1999b]
            logAreaLum = 0.0;
            for ( int i = 0; scenePatches != NULL && i < scenePatches->size(); i++ ) {
                Adaptation::patchComputeLogAreaLum(scenePatches->get(i), toneMapOptions.getColorSpace());
            }
            // Equation [TUMB1999b](7): convert mean log-luminance back to luminance domain
            toneMapOptions.realWorldAdaptionLuminance = Math::exp(((double)(logAreaLum)) / Statistics::instance().radiance.totalArea + 0.84);
            break;
        }
        case TMA_MEDIAN: {
            // Static adaptation inspired by [TUMB1999b]
            LuminanceArea *la = new LuminanceArea[Statistics::instance().reader.numberOfPatches];

            lumArea = la;
            lumAreaIndex = 0;
            numEntries = 0;
            for ( int i = 0; scenePatches != NULL && i < scenePatches->size(); i++ ) {
                Adaptation::patchFillLumArea(scenePatches->get(i), toneMapOptions.getColorSpace());
            }
            toneMapOptions.realWorldAdaptionLuminance = Adaptation::meanAreaWeightedLuminance(la, Statistics::instance().reader.numberOfPatches);

            delete[] la;
            break;
        }
        default:
            Logger::reportMessage("Adaptation", Logger::ERROR, "sceneBuilderComputeStats", LoggerFormat::format("unknown static adaptation method %d", toneMapOptions.staticAdaptationMethod));
    }
}

/**
Same as Adaptation::estimateSceneAdaptation, but uses some a-priori estimate for the radiance emitted by a patch.
Used when loading a new scene
*/
void
Adaptation::initSceneAdaptation(
    const ArrayList<Patch *> *scenePatches,
    ToneMappingContext &toneMapOptions)
{
    Adaptation::estimateSceneAdaptation(Adaptation::initRadianceEstimate, scenePatches, toneMapOptions);
}
