#ifndef __POTENTIAL__
#define __POTENTIAL__

#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "skin/Patch.h"
#include "scene/Camera.h"
#include "scene/Scene.h"

extern void updateDirectPotential(Scene *scene, RenderOptions *renderOptions);
extern void updateDirectVisibility(Scene *scene, RenderOptions *renderOptions);

#endif
