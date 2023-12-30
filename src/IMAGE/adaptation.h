/* adaptation.h: estimate static adaptation luminance in the current scene */

#ifndef _RPK_ADAPTATION_H_
#define _RPK_ADAPTATION_H_

#include "skin/patch.h"

/* Static adaptation estimation methods */
enum TMA_METHOD {
    TMA_NONE, TMA_AVERAGE, TMA_MEDIAN, TMA_IDRENDER
};

/* estimates adaptation luminance in the current scene using the current
 * adaption estimation method in tmopts.statadapt (tonemapping.h).
 * 'patch_radiance' is a pointer to a routine that computes the radiance
 * emitted by a patch. The result is filled in tmopts.lwa. */
extern void EstimateSceneAdaptation(COLOR (*patch_radiance)(PATCH *));

/* same, but uses some a-priori estimate for the radiance emitted by a patch.
 * Used when laoding a new scene. */
extern void InitSceneAdaptation();

#endif
