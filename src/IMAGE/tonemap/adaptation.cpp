/**
Estimate static adaptation for tone mapping
*/

#include <cfloat>

#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/Statistics.h"
#include "IMAGE/tonemap/adaptation.h"
#include "IMAGE/tonemap/ToneMap.h"

/**
Stores luminance-area pairs for median area-weighted luminance
selection.
*/
class LuminanceArea {
  public:
    float luminance;
    float area;
};

/**
A-priori estimate of a patch's radiance
*/
static ColorRgb
initRadianceEstimate(Patch *patch) {
    ColorRgb E = patch->averageEmittance(ALL_COMPONENTS);
    ColorRgb R = patch->averageNormalAlbedo(BSDF_ALL_COMPONENTS);
    ColorRgb radiance;

    radiance.scalarProduct(R, GLOBAL_statistics.estimatedAverageRadiance);
    radiance.addScaled(radiance, (1.0f / (float)M_PI), E);
    return radiance;
}

static int globalNumEntries;
static double globalLogAreaLum;
static LuminanceArea *globalLumArea;
static float globalLumMin = FLT_MAX; // Note Numeric::HUGE_FLOAT_VALUE; will cause an issue here
static float globalLumMax = 0.0;
static ColorRgb (*PatchRadianceEstimate)(Patch *globalP) = initRadianceEstimate;

static int
adaptationLumAreaComp(const void *la1, const void *la2) {
    float l1 = ((const LuminanceArea *) la1)->luminance;
    float l2 = ((const LuminanceArea *) la2)->luminance;

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
    globalLogAreaLum += patch->area * java::Math::log(brightness);
}

static void
patchFillLumArea(Patch *patch) {
    float brightness = patchBrightnessEstimate(patch);

    globalLumArea->luminance = brightness;
    globalLumArea->area = patch->area;

    globalLumMin = java::Math::min(globalLumMin, globalLumArea->luminance);
    globalLumMax = java::Math::max(globalLumMax, globalLumArea->luminance);

    globalLumArea++;
    globalNumEntries++;
}

/**
Computes the static adaptation luminance value choosing the median value
of area-weighted luminance values. Needs correct value of "GLOBAL_statistics_totalArea".
*/
static float
meanAreaWeightedLuminance(LuminanceArea *pairs, int numPairs) {
    float areaMax = GLOBAL_statistics.totalArea / 2.0f;
    float areaCnt = 0.0;

    qsort((void *) pairs, numPairs, sizeof(LuminanceArea), (QSORT_CALLBACK_TYPE) adaptationLumAreaComp);

    while ( areaCnt < areaMax ) {
        areaCnt += pairs->area;
        pairs++;
    }

    pairs--;

    return pairs->luminance;
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
        case TMA_NONE:
            break;
        case TMA_AVERAGE: {
            // Gibson's static adaptation after [TUMB1999b]
            globalLogAreaLum = 0.0;
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                patchComputeLogAreaLum(scenePatches->get(i));
            }
            GLOBAL_toneMap_options.realWorldAdaptionLuminance = java::Math::exp((float)globalLogAreaLum / GLOBAL_statistics.totalArea + 0.84f);
            break;
        }
        case TMA_MEDIAN: {
            // Static adaptation inspired by [TUMB1999b]
            LuminanceArea *la = new LuminanceArea[GLOBAL_statistics.numberOfPatches];

            globalLumArea = la;
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                patchFillLumArea(scenePatches->get(i));
            }
            GLOBAL_toneMap_options.realWorldAdaptionLuminance = meanAreaWeightedLuminance(la, GLOBAL_statistics.numberOfPatches);

            delete[] la;
            break;
        }
        default:
            logError("sceneBuilderComputeStats", "unknown static adaptation method %d", GLOBAL_toneMap_options.staticAdaptationMethod);
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
