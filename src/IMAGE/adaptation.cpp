/* adaptation.c: estimate static adaptation for tone mapping */

#include <cstdlib>

#include "common/error.h"
#include "material/statistics.h"
#include "scene/scene.h"
#include "IMAGE/adaptation.h"
#include "IMAGE/tonemap/tonemapping.h"

/* ---------------------------------------------------------------------------
  `LUMAREA'

  Stores luminance-area pairs for median area-weighted luminance
  selection.
  ------------------------------------------------------------------------- */
class LUMAREA {
  public:
    float luminance;
    float area;
};

static int _lumAreaComp(const void *la1, const void *la2) {
    float l1 = ((LUMAREA *) la1)->luminance;
    float l2 = ((LUMAREA *) la2)->luminance;
    return l1 > l2 ? 1 : l1 == l2 ? 0 : -1;
}

/* ---------------------------------------------------------------------------
  `MeanAreaWeightedLuminance'

  Computes the static adaptation luminance value choosing the median value
  of area-weighted luminance values. Needs correct value of "GLOBAL_statistics_totalArea".
  ------------------------------------------------------------------------- */
float MeanAreaWeightedLuminance(LUMAREA *pairs, int numPairs) {
    float areaMax = GLOBAL_statistics_totalArea / 2.0;
    float areaCnt = 0.0;

    qsort((void *) pairs, numPairs, sizeof(LUMAREA), (QSORT_CALLBACK_TYPE)_lumAreaComp);

    while ( areaCnt < areaMax ) {
        areaCnt += pairs->area;
        pairs++;
    }

    pairs--;

    return pairs->luminance;
}

/* Global variables local to this module */
static int _numEntries;
static double _logAreaLum;
static LUMAREA *_lumArea;
static float _lumMin = HUGE;
static float _lumMax = 0.0;

/* a-priori estimate of a patch's radiance. */
static COLOR InitRadianceEstimate(PATCH *patch) {
    COLOR E = PatchAverageEmittance(patch, ALL_COMPONENTS),
            R = PatchAverageNormalAlbedo(patch, BSDF_ALL_COMPONENTS),
            radiance;

    COLORPROD(R, GLOBAL_statistics_estimatedAverageRadiance, radiance);
    COLORADDSCALED(radiance, (1. / M_PI), E, radiance);
    return radiance;
}

static COLOR (*PatchRadianceEstimate)(PATCH *P) = InitRadianceEstimate;

static float PatchBrightnessEstimate(PATCH *patch) {
    COLOR radiance = PatchRadianceEstimate(patch);
    float brightness = ColorLuminance(radiance);
    if ( brightness < EPSILON ) {
        brightness = EPSILON;
    }
    return brightness;
}

static void PatchComputeLogAreaLum(PATCH *patch) {
    float brightness = PatchBrightnessEstimate(patch);
    _logAreaLum += patch->area * log(brightness);
}

static void PatchFillLumArea(PATCH *patch) {
    float brightness = PatchBrightnessEstimate(patch);

    _lumArea->luminance = brightness;
    _lumArea->area = patch->area;

    _lumMin = MIN(_lumMin, _lumArea->luminance);
    _lumMax = MAX(_lumMax, _lumArea->luminance);

    _lumArea++;
    _numEntries++;
}

/* for ReEstimateAdaptation() below */
static int last_is_scene = true;

void EstimateSceneAdaptation(COLOR (*patch_radiance)(PATCH *)) {
    PatchRadianceEstimate = patch_radiance;
    last_is_scene = true;

    switch ( tmopts.statadapt ) {
        case TMA_NONE:
            break;
        case TMA_AVERAGE: {
            /* Gibson's static adpatation after Tumblin[1993] */
            _logAreaLum = 0.0;
            PatchListIterate(GLOBAL_scene_patches, PatchComputeLogAreaLum);
            tmopts.lwa = exp(_logAreaLum / GLOBAL_statistics_totalArea + 0.84);
            break;
        }
        case TMA_MEDIAN: {
            /* Static adaptation inspired by Tumblin[1999] */
            LUMAREA *la = (LUMAREA *)malloc(GLOBAL_statistics_numberOfPatches * sizeof(LUMAREA));

            _lumArea = la;
            PatchListIterate(GLOBAL_scene_patches, PatchFillLumArea);
            tmopts.lwa = MeanAreaWeightedLuminance(la, GLOBAL_statistics_numberOfPatches);

            free(la);
            break;
        }
        default:
            Error("ComputeSomeSceneStats", "unknown static adaptation method %d", tmopts.statadapt);
    }
}

void InitSceneAdaptation() {
    EstimateSceneAdaptation(InitRadianceEstimate);
}
