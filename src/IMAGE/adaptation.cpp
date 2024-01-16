/**
Estimate static adaptation for tone mapping
*/

#include "common/error.h"
#include "material/statistics.h"
#include "scene/scene.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "IMAGE/adaptation.h"

/**
Stores luminance-area pairs for median area-weighted luminance
selection.
*/
class LUMAREA {
  public:
    float luminance;
    float area;
};

/**
A-priori estimate of a patch's radiance
*/
static COLOR
initRadianceEstimate(Patch *patch) {
    COLOR E = patchAverageEmittance(patch, ALL_COMPONENTS),
            R = patchAverageNormalAlbedo(patch, BSDF_ALL_COMPONENTS),
            radiance;

    colorProduct(R, GLOBAL_statistics_estimatedAverageRadiance, radiance);
    colorAddScaled(radiance, (1.0 / M_PI), E, radiance);
    return radiance;
}

static int globalNumEntries;
static double globalLogAreaLum;
static LUMAREA *globalLumArea;
static float globalLumMin = HUGE;
static float globalLumMax = 0.0;
static COLOR (*PatchRadianceEstimate)(Patch *globalP) = initRadianceEstimate;

static int
adaptationLumAreaComp(const void *la1, const void *la2) {
    float l1 = ((LUMAREA *) la1)->luminance;
    float l2 = ((LUMAREA *) la2)->luminance;
    return l1 > l2 ? 1 : l1 == l2 ? 0 : -1;
}

static float
patchBrightnessEstimate(Patch *patch) {
    COLOR radiance = PatchRadianceEstimate(patch);
    float brightness = colorLuminance(radiance);
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

    globalLumMin = MIN(globalLumMin, globalLumArea->luminance);
    globalLumMax = MAX(globalLumMax, globalLumArea->luminance);

    globalLumArea++;
    globalNumEntries++;
}

/**
Computes the static adaptation luminance value choosing the median value
of area-weighted luminance values. Needs correct value of "GLOBAL_statistics_totalArea".
*/
static float
meanAreaWeightedLuminance(LUMAREA *pairs, int numPairs) {
    float areaMax = GLOBAL_statistics_totalArea / 2.0f;
    float areaCnt = 0.0;

    qsort((void *) pairs, numPairs, sizeof(LUMAREA), (QSORT_CALLBACK_TYPE) adaptationLumAreaComp);

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
estimateSceneAdaptation(COLOR (*patch_radiance)(Patch *)) {
    PatchRadianceEstimate = patch_radiance;

    switch ( GLOBAL_toneMap_options.statadapt ) {
        case TMA_NONE:
            break;
        case TMA_AVERAGE: {
            // Gibson's static adaptation after Tumblin[1993]
            globalLogAreaLum = 0.0;
            for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
                patchComputeLogAreaLum(window->patch);
            }
            GLOBAL_toneMap_options.lwa = (float)std::exp(globalLogAreaLum / GLOBAL_statistics_totalArea + 0.84);
            break;
        }
        case TMA_MEDIAN: {
            // Static adaptation inspired by Tumblin[1999]
            LUMAREA *la = (LUMAREA *)malloc(GLOBAL_statistics_numberOfPatches * sizeof(LUMAREA));

            globalLumArea = la;
            for ( PatchSet *window = GLOBAL_scene_patches; window != nullptr; window = window->next ) {
                patchFillLumArea(window->patch);
            }
            GLOBAL_toneMap_options.lwa = meanAreaWeightedLuminance(la, GLOBAL_statistics_numberOfPatches);

            free(la);
            break;
        }
        default:
            logError("computeSomeSceneStats", "unknown static adaptation method %d", GLOBAL_toneMap_options.statadapt);
    }
}

/**
Same as estimateSceneAdaptation, but uses some a-priori estimate for the radiance emitted by a patch.
Used when loading a new scene
*/
void
initSceneAdaptation() {
    estimateSceneAdaptation(initRadianceEstimate);
}
