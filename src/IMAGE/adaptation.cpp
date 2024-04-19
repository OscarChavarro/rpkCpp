/**
Estimate static adaptation for tone mapping
*/

#include "java/util/ArrayList.txx"
#include "common/mymath.h"
#include "common/error.h"
#include "material/statistics.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "IMAGE/adaptation.h"

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
    radiance.addScaled(radiance, (1.0 / M_PI), E);
    return radiance;
}

static int globalNumEntries;
static double globalLogAreaLum;
static LuminanceArea *globalLumArea;
static float globalLumMin = HUGE;
static float globalLumMax = 0.0;
static ColorRgb (*PatchRadianceEstimate)(Patch *globalP) = initRadianceEstimate;

static int
adaptationLumAreaComp(const void *la1, const void *la2) {
    float l1 = ((LuminanceArea *) la1)->luminance;
    float l2 = ((LuminanceArea *) la2)->luminance;
    return l1 > l2 ? 1 : l1 == l2 ? 0 : -1;
}

static float
patchBrightnessEstimate(Patch *patch) {
    ColorRgb radiance = PatchRadianceEstimate(patch);
    float brightness = radiance.luminance();
    if ( brightness < EPSILON ) {
        brightness = EPSILON;
    }
    return brightness;
}

static void
patchComputeLogAreaLum(Patch *patch) {
    float brightness = patchBrightnessEstimate(patch);
    globalLogAreaLum += patch->area * std::log(brightness);
}

static void
patchFillLumArea(Patch *patch) {
    float brightness = patchBrightnessEstimate(patch);

    globalLumArea->luminance = brightness;
    globalLumArea->area = patch->area;

    globalLumMin = floatMin(globalLumMin, globalLumArea->luminance);
    globalLumMax = floatMax(globalLumMax, globalLumArea->luminance);

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
estimateSceneAdaptation(ColorRgb (*patch_radiance)(Patch *), java::ArrayList<Patch *> *scenePatches) {
    PatchRadianceEstimate = patch_radiance;

    switch ( GLOBAL_toneMap_options.staticAdaptationMethod ) {
        case TMA_NONE:
            break;
        case TMA_AVERAGE: {
            // Gibson's static adaptation after Tumblin[1993]
            globalLogAreaLum = 0.0;
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                patchComputeLogAreaLum(scenePatches->get(i));
            }
            GLOBAL_toneMap_options.realWorldAdaptionLuminance = (float)std::exp(globalLogAreaLum / GLOBAL_statistics.totalArea + 0.84);
            break;
        }
        case TMA_MEDIAN: {
            // Static adaptation inspired by Tumblin[1999]
            LuminanceArea *la = (LuminanceArea *)malloc(GLOBAL_statistics.numberOfPatches * sizeof(LuminanceArea));

            globalLumArea = la;
            for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
                patchFillLumArea(scenePatches->get(i));
            }
            GLOBAL_toneMap_options.realWorldAdaptionLuminance = meanAreaWeightedLuminance(la, GLOBAL_statistics.numberOfPatches);

            free(la);
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
initSceneAdaptation(java::ArrayList<Patch *> *scenePatches) {
    estimateSceneAdaptation(initRadianceEstimate, scenePatches);
}
