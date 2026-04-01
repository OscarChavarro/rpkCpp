/**
Estimate static adaptation for tone mapping
*/

#include <cstdlib>

#include "java/lang/Float.h"
#include "java/util/ArrayList.txx"
#include "common/Error.h"
#include "common/statistics/Statistics.h"
#include "tonemap/ToneMap.h"
#include "numericalAnalysis/PatchVisitor.h"
#include "app/Adaptation.h"
#include "app/LuminanceArea.h"

static int globalNumEntries;
static double globalLogAreaLum;
static LuminanceArea *globalLumArea;
static int globalLumAreaIndex;
static float globalLumMin = java::Float::MAX_VALUE; // Note Numeric::HUGE_FLOAT_VALUE; will cause an issue here
static float globalLumMax = 0.0;
static ColorRgb (*PatchRadianceEstimate)(Patch *globalP) = nullptr;

/**
A-priori estimate of a patch's radiance
*/
ColorRgb
Adaptation::initRadianceEstimate(Patch *patch) {
    ColorRgb E = PatchVisitor::averageEmittance(patch, ALL_COMPONENTS);
    ColorRgb R = PatchVisitor::averageNormalAlbedo(patch, BSDF_ALL_COMPONENTS);
    ColorRgb radiance;

    radiance.scalarProduct(R, Statistics::instance().radiance.estimatedAverageRadiance);
    radiance.addScaled(radiance, (1.0f / static_cast<float>(M_PI)), E);
    return radiance;
}

int
Adaptation::adaptationLumAreaComp(const void *la1, const void *la2) {
    float l1 = static_cast<const LuminanceArea *>(la1)->luminance;
    float l2 = static_cast<const LuminanceArea *>(la2)->luminance;

    if ( l1 > l2 ) {
        return 1;
    }

    return l1 == l2 ? 0 : -1;
}

float
Adaptation::patchBrightnessEstimate(Patch *patch) {
    ColorRgb radiance = PatchRadianceEstimate(patch);
    float brightness = radiance.luminance();
    if ( brightness < Numeric::EPSILON_FLOAT ) {
        brightness = Numeric::EPSILON_FLOAT;
    }
    return brightness;
}

void
Adaptation::patchComputeLogAreaLum(Patch *patch) {
    float brightness = Adaptation::patchBrightnessEstimate(patch);
    // Equation [TUMB1999b](7): log(Lwa) as mean(log(Lw)), here area-weighted over patches
    globalLogAreaLum += patch->area * java::Math::log(brightness);
}

void
Adaptation::patchFillLumArea(Patch *patch) {
    float brightness = Adaptation::patchBrightnessEstimate(patch);

    LuminanceArea &entry = globalLumArea[globalLumAreaIndex];
    entry.luminance = brightness;
    entry.area = patch->area;

    globalLumMin = java::Math::min(globalLumMin, entry.luminance);
    globalLumMax = java::Math::max(globalLumMax, entry.luminance);

    globalLumAreaIndex++;
    globalNumEntries++;
}

/**
Computes the static adaptation luminance value choosing the median value
of area-weighted luminance values. Needs correct value of "GLOBAL_statistics_totalArea".
*/
float
Adaptation::meanAreaWeightedLuminance(LuminanceArea *pairs, int numPairs) {
    if ( numPairs <= 0 ) {
        return 0.0f;
    }

    float areaMax = Statistics::instance().radiance.totalArea / 2.0f;
    float areaCnt = 0.0;
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
    ColorRgb (*patch_radiance)(Patch *),
    const java::ArrayList<Patch *> *scenePatches,
    ToneMappingContext &toneMapOptions)
{
    PatchRadianceEstimate = patch_radiance;

    switch ( toneMapOptions.staticAdaptationMethod ) {
        case ToneMapAdaptationMethod::TMA_NONE:
            break;
        case ToneMapAdaptationMethod::TMA_AVERAGE: {
            // Gibson's static adaptation after [TUMB1999b]
            globalLogAreaLum = 0.0;
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                Adaptation::patchComputeLogAreaLum(scenePatches->get(i));
            }
            // Equation [TUMB1999b](7): convert mean log-luminance back to luminance domain
            toneMapOptions.realWorldAdaptionLuminance = java::Math::exp(static_cast<float>(globalLogAreaLum) / Statistics::instance().radiance.totalArea + 0.84f);
            break;
        }
        case ToneMapAdaptationMethod::TMA_MEDIAN: {
            // Static adaptation inspired by [TUMB1999b]
            LuminanceArea *la = new LuminanceArea[Statistics::instance().reader.numberOfPatches];

            globalLumArea = la;
            globalLumAreaIndex = 0;
            globalNumEntries = 0;
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                Adaptation::patchFillLumArea(scenePatches->get(i));
            }
            toneMapOptions.realWorldAdaptionLuminance = Adaptation::meanAreaWeightedLuminance(la, Statistics::instance().reader.numberOfPatches);

            delete[] la;
            break;
        }
        default:
            Error::error("sceneBuilderComputeStats", "unknown static adaptation method %d", toneMapOptions.staticAdaptationMethod);
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
