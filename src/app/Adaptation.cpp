/**
Estimate static adaptation for tone mapping
*/

#include "java/lang/Float.h"
#include "java/util/ArrayList.txx"
#include "common/Error.h"
#include "common/Statistics.h"
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

/**
A-priori estimate of a patch's radiance
*/
static ColorRgb
initRadianceEstimate(Patch *patch) {
    ColorRgb E = PatchVisitor::averageEmittance(patch, ALL_COMPONENTS);
    ColorRgb R = PatchVisitor::averageNormalAlbedo(patch, BSDF_ALL_COMPONENTS);
    ColorRgb radiance;

    radiance.scalarProduct(R, GLOBAL_statistics.estimatedAverageRadiance);
    radiance.addScaled(radiance, (1.0f / static_cast<float>(M_PI)), E);
    return radiance;
}

static ColorRgb (*PatchRadianceEstimate)(Patch *globalP) = initRadianceEstimate;

static int
adaptationLumAreaComp(const void *la1, const void *la2) {
    float l1 = static_cast<const LuminanceArea *>(la1)->luminance;
    float l2 = static_cast<const LuminanceArea *>(la2)->luminance;

    if ( l1 > l2 ) {
        return 1;
    }

    return l1 == l2 ? 0 : -1;
}

static float
patchBrightnessEstimate(Patch *patch) {
    ColorRgb radiance = PatchRadianceEstimate(patch);
    float brightness = radiance.luminance();
    if ( brightness < Numeric::EPSILON_FLOAT ) {
        brightness = Numeric::EPSILON_FLOAT;
    }
    return brightness;
}

static void
patchComputeLogAreaLum(Patch *patch) {
    float brightness = patchBrightnessEstimate(patch);
    // Equation [TUMB1999b](7): log(Lwa) as mean(log(Lw)), here area-weighted over patches
    globalLogAreaLum += patch->area * java::Math::log(brightness);
}

static void
patchFillLumArea(Patch *patch) {
    float brightness = patchBrightnessEstimate(patch);

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
static float
meanAreaWeightedLuminance(LuminanceArea *pairs, int numPairs) {
    if ( numPairs <= 0 ) {
        return 0.0f;
    }

    float areaMax = GLOBAL_statistics.totalArea / 2.0f;
    float areaCnt = 0.0;
    int pairIndex = 0;

    qsort(pairs, numPairs, sizeof(LuminanceArea), adaptationLumAreaComp);

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
adaption estimation method in GLOBAL_toneMap_options.statadapt
'patch_radiance' is a pointer to a routine that computes the radiance
emitted by a patch. The result is filled in GLOBAL_toneMap_options.lwa
*/
static void
estimateSceneAdaptation(ColorRgb (*patch_radiance)(Patch *), const java::ArrayList<Patch *> *scenePatches) {
    PatchRadianceEstimate = patch_radiance;

    switch ( GLOBAL_toneMap_options.staticAdaptationMethod ) {
        case ToneMapAdaptationMethod::TMA_NONE:
            break;
        case ToneMapAdaptationMethod::TMA_AVERAGE: {
            // Gibson's static adaptation after [TUMB1999b]
            globalLogAreaLum = 0.0;
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                patchComputeLogAreaLum(scenePatches->get(i));
            }
            // Equation [TUMB1999b](7): convert mean log-luminance back to luminance domain
            GLOBAL_toneMap_options.realWorldAdaptionLuminance = java::Math::exp(static_cast<float>(globalLogAreaLum) / GLOBAL_statistics.totalArea + 0.84f);
            break;
        }
        case ToneMapAdaptationMethod::TMA_MEDIAN: {
            // Static adaptation inspired by [TUMB1999b]
            LuminanceArea *la = new LuminanceArea[GLOBAL_statistics.numberOfPatches];

            globalLumArea = la;
            globalLumAreaIndex = 0;
            globalNumEntries = 0;
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                patchFillLumArea(scenePatches->get(i));
            }
            GLOBAL_toneMap_options.realWorldAdaptionLuminance = meanAreaWeightedLuminance(la, GLOBAL_statistics.numberOfPatches);

            delete[] la;
            break;
        }
        default:
            Error::error("sceneBuilderComputeStats", "unknown static adaptation method %d", GLOBAL_toneMap_options.staticAdaptationMethod);
    }
}

/**
Same as estimateSceneAdaptation, but uses some a-priori estimate for the radiance emitted by a patch.
Used when loading a new scene
*/
void
initSceneAdaptation(const java::ArrayList<Patch *> *scenePatches) {
    estimateSceneAdaptation(initRadianceEstimate, scenePatches);
}
