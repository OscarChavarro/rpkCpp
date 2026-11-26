/**
Estimate static adaptation luminance in the current scene
*/

#ifndef __ADAPTATION__
#define __ADAPTATION__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "LuminanceArea.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

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
    static double lumMin;
    static double lumMax;
    static PatchRadianceEstimateFn patchRadianceEstimate;

    static ColorRgb initRadianceEstimate(Patch *patch);
    static int adaptationLumAreaComp(const void *la1, const void *la2);
    static double patchBrightnessEstimate(Patch *patch, const Cie &colorSpace);
    static void patchComputeLogAreaLum(Patch *patch, const Cie &colorSpace);
    static void patchFillLumArea(Patch *patch, const Cie &colorSpace);
    static double meanAreaWeightedLuminance(LuminanceArea *pairs, int numPairs);
    static void estimateSceneAdaptation(
        PatchRadianceEstimateFn patchRadiance,
        const java::ArrayList<Patch *> *scenePatches,
        ToneMappingContext &toneMapOptions);
};

#endif
