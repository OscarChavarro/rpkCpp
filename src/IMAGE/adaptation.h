/**
Estimate static adaptation luminance in the current scene
*/

#ifndef __ADAPTATION__
#define __ADAPTATION__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"

/**
Static adaptation estimation methods
*/
enum TMA_METHOD {
    TMA_NONE,
    TMA_AVERAGE,
    TMA_MEDIAN
};

extern void initSceneAdaptation(java::ArrayList<Patch *> *scenePatches);

#endif
