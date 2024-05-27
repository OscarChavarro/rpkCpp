/**
Estimate static adaptation luminance in the current scene
*/

#ifndef __ADAPTATION__
#define __ADAPTATION__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"

extern void initSceneAdaptation(const java::ArrayList<Patch *> *scenePatches);

#endif
