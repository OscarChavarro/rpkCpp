/**
Estimate static adaptation luminance in the current scene
*/

#ifndef __ADAPTATION__
#define __ADAPTATION__

#include "java/util/ArrayList.h"
#include "common/color/ColorRgb.h"
#include "environment/geometry/elements/Patch.h"
#include "app/LuminanceArea.h"
#include "tonemap/ToneMappingContext.h"

class Adaptation final {
  public:
    static void initSceneAdaptation(
        const java::ArrayList<Patch *> *scenePatches,
        ToneMappingContext &toneMapOptions);

  private:
    using PatchRadianceEstimateFn = ColorRgb (*)(Patch *);

    static int numEntries;
    static double logAreaLum;
    static LuminanceArea *lumArea;
    static int lumAreaIndex;
    static float lumMin;
    static float lumMax;
    static PatchRadianceEstimateFn patchRadianceEstimate;

    static ColorRgb initRadianceEstimate(Patch *patch);
    static int adaptationLumAreaComp(const void *la1, const void *la2);
    static float patchBrightnessEstimate(Patch *patch);
    static void patchComputeLogAreaLum(Patch *patch);
    static void patchFillLumArea(Patch *patch);
    static float meanAreaWeightedLuminance(LuminanceArea *pairs, int numPairs);
    static void estimateSceneAdaptation(
        PatchRadianceEstimateFn patchRadiance,
        const java::ArrayList<Patch *> *scenePatches,
        ToneMappingContext &toneMapOptions);
};

#endif
