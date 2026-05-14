/**
Estimate static adaptation luminance in the current scene
*/

#ifndef ADAPTATION__
#define ADAPTATION__

#include "vsdk/toolkit/java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "LuminanceArea.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

class Adaptation final {
  public:
    static void initSceneAdaptation(
        const java::ArrayList<Patch *> *scenePatches,
        ToneMappingContext &toneMapOptions);

  private:
    using PatchRadianceEstimateFn = ColorRgbMutable (*)(Patch *);

    static int numEntries;
    static double logAreaLum;
    static LuminanceArea *lumArea;
    static int lumAreaIndex;
    static float lumMin;
    static float lumMax;
    static PatchRadianceEstimateFn patchRadianceEstimate;

    static ColorRgbMutable initRadianceEstimate(Patch *patch);
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
