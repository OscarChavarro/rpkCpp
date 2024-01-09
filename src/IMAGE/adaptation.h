/* adaptation.h: estimate static adaptation luminance in the current scene */

#ifndef _RPK_ADAPTATION_H_
#define _RPK_ADAPTATION_H_

#include "skin/Patch.h"

/* Static adaptation estimation methods */
enum TMA_METHOD {
    TMA_NONE, TMA_AVERAGE, TMA_MEDIAN, TMA_IDRENDER
};

extern void initSceneAdaptation();

#endif
