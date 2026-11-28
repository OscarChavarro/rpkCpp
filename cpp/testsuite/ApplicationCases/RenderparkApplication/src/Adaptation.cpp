/**
Estimate static adaptation for tone mapping
*/

#include <cstdlib>

#include "java/lang/Math.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"
#include "vsdk/toolkit/numericalAnalysis/PatchVisitor.h"
#include "Adaptation.h"
#include "LuminanceArea.h"

int Adaptation::numEntries = 0;
double Adaptation::logAreaLum = 0.0;
LuminanceArea *Adaptation::lumArea = nullptr;
int Adaptation::lumAreaIndex = 0;
double Adaptation::lumMin = java::Double::MAX_VALUE; // Note java::Double::MAX_VALUE; will cause an issue here
double Adaptation::lumMax = 0.0;
Adaptation::PatchRadianceEstimateFn Adaptation::patchRadianceEstimate = nullptr;

/**
A-priori estimate of a patch's radiance
*/
ColorRgb
Adaptation::initRadianceEstimate(Patch *patch) {
    const ColorRgb E = PatchVisitor::averageEmittance(patch, XxdfComponentFlagInfo::ALL_COMPONENTS);
    const ColorRgb R = PatchVisitor::averageNormalAlbedo(patch, BsdfComponentInfo::BSDF_ALL_COMPONENTS);
    const ColorRgb &average = Statistics::instance().radiance.estimatedAverageRadiance;
    const double scale = 1.0 / static_cast<double>(M_PI);
    return ColorRgb(R.getR() * average.getR() + scale * E.getR(),
                    R.getG() * average.getG() + scale * E.getG(),
                    R.getB() * average.getB() + scale * E.getB());
}

int
Adaptation::adaptationLumAreaComp(const void *la1, const void *la2) {
    const double l1 = static_cast<const LuminanceArea *>(la1)->luminance;
    const double l2 = static_cast<const LuminanceArea *>(la2)->luminance;

    if ( l1 > l2 ) {
        return 1;
    }

    return l1 == l2 ? 0 : -1;
}

double
Adaptation::patchBrightnessEstimate(Patch *patch, const Cie &colorSpace) {
    const ColorRgb radiance = patchRadianceEstimate(patch);
    double brightness = static_cast<double>(colorSpace.spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB()));
    if ( brightness < Numeric::EPSILON ) {
        brightness = Numeric::EPSILON;
    }
    return brightness;
}

void
Adaptation::patchComputeLogAreaLum(Patch *patch, const Cie &colorSpace) {
    const double brightness = Adaptation::patchBrightnessEstimate(patch, colorSpace);
    // Equation [TUMB1999b](7): log(Lwa) as mean(log(Lw)), here area-weighted over patches
    logAreaLum += patch->getArea() * java::Math::log(brightness);
}

void
Adaptation::patchFillLumArea(Patch *patch, const Cie &colorSpace) {
    const double brightness = Adaptation::patchBrightnessEstimate(patch, colorSpace);

    LuminanceArea &entry = lumArea[lumAreaIndex];
    entry.luminance = brightness;
    entry.area = patch->getArea();

    lumMin = java::Math::min(lumMin, entry.luminance);
    lumMax = java::Math::max(lumMax, entry.luminance);

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

    const double areaMax = Statistics::instance().radiance.totalArea / 2.0;
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
    const java::ArrayList<Patch *> *scenePatches,
    ToneMappingContext &toneMapOptions)
{
    patchRadianceEstimate = patchRadiance;

    switch ( toneMapOptions.staticAdaptationMethod ) {
        case ToneMapAdaptationMethod::TMA_NONE:
            break;
        case ToneMapAdaptationMethod::TMA_AVERAGE: {
            // Gibson's static adaptation after [TUMB1999b]
            logAreaLum = 0.0;
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                Adaptation::patchComputeLogAreaLum(scenePatches->get(i), toneMapOptions.getColorSpace());
            }
            // Equation [TUMB1999b](7): convert mean log-luminance back to luminance domain
            toneMapOptions.realWorldAdaptionLuminance = java::Math::exp(static_cast<double>(logAreaLum) / Statistics::instance().radiance.totalArea + 0.84);
            break;
        }
        case ToneMapAdaptationMethod::TMA_MEDIAN: {
            // Static adaptation inspired by [TUMB1999b]
            LuminanceArea * const la = new LuminanceArea[Statistics::instance().reader.numberOfPatches];

            lumArea = la;
            lumAreaIndex = 0;
            numEntries = 0;
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                Adaptation::patchFillLumArea(scenePatches->get(i), toneMapOptions.getColorSpace());
            }
            toneMapOptions.realWorldAdaptionLuminance = Adaptation::meanAreaWeightedLuminance(la, Statistics::instance().reader.numberOfPatches);

            delete[] la;
            break;
        }
        default:
            Logger::reportMessage("Adaptation", Logger::ERROR, "sceneBuilderComputeStats", java::String::format("unknown static adaptation method %d", toneMapOptions.staticAdaptationMethod));
    }
}

/**
Same as Adaptation::estimateSceneAdaptation, but uses some a-priori estimate for the radiance emitted by a patch.
Used when loading a new scene
*/
void
Adaptation::initSceneAdaptation(
    const java::ArrayList<Patch *> *scenePatches,
    ToneMappingContext &toneMapOptions)
{
    Adaptation::estimateSceneAdaptation(Adaptation::initRadianceEstimate, scenePatches, toneMapOptions);
}
