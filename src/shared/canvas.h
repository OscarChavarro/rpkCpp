#ifndef _CANVAS_H_INCLUDED_
#define _CANVAS_H_INCLUDED_

#include "java/util/ArrayList.h"
#include "skin/Patch.h"

extern void createOffscreenCanvasWindow(int width, int height, java::ArrayList<Patch *> *scenePatches);
extern void canvasPushMode();
extern void canvasPullMode();

#endif
