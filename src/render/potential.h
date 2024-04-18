#ifndef __POTENTIAL__
#define __POTENTIAL__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "scene/Camera.h"

extern void
updateDirectPotential(
    Camera *camera,
    java::ArrayList<Patch *> *scenePatches,
    Geometry *clusteredWorldGeometry);

extern void
updateDirectVisibility(
    Camera *camera,
    java::ArrayList<Patch *> *scenePatches,
    Geometry *clusteredWorldGeometry);

#endif
