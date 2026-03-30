/**
Estimate static adaptation luminance in the current scene
*/

#ifndef __ADAPTATION__
#define __ADAPTATION__

#include "java/util/ArrayList.h"
#include "common/ColorRgb.h"
#include "skin/Patch.h"
#include "app/LuminanceArea.h"

class ToneMappingContext;

class Adaptation final {
  public:
    static void initSceneAdaptation(
        const java::ArrayList<Patch *> *scenePatches,
        ToneMappingContext &toneMapOptions);

  private:
    static ColorRgb initRadianceEstimate(Patch *patch);
    static int adaptationLumAreaComp(const void *la1, const void *la2);
    static float patchBrightnessEstimate(Patch *patch);
    static void patchComputeLogAreaLum(Patch *patch);
    static void patchFillLumArea(Patch *patch);
    static float meanAreaWeightedLuminance(LuminanceArea *pairs, int numPairs);
    static void estimateSceneAdaptation(
        ColorRgb (*patchRadiance)(Patch *),
        const java::ArrayList<Patch *> *scenePatches,
        ToneMappingContext &toneMapOptions);
};

#endif
