/**
Software ID rendering: because hardware ID rendering is tricky 
due to frame buffer formats, etc.
*/

#ifndef __SOFT_IDS__
#define __SOFT_IDS__

#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "SGL/sgl.h"
#include "skin/Patch.h"
#include "scene/Camera.h"
#include "scene/Scene.h"

extern SGL_CONTEXT *setupSoftFrameBuffer(Camera *camera);
extern void softRenderPatches(Scene *scene, RenderOptions *renderOptions);
extern unsigned long * softRenderIds(long *x, long *y, Scene *scene);
extern unsigned long *sglRenderIds(long *x, long *y, Scene *scene);

#endif
