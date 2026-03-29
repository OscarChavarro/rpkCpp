#ifndef __POTENTIAL__
#define __POTENTIAL__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "scene/Camera.h"
#include "scene/Scene.h"

extern void updateDirectPotential(const Scene *scene, const RenderOptions *renderOptions);
extern void updateDirectVisibility(const Scene *scene, const RenderOptions *renderOptions);

#endif
