#ifndef __POTENTIAL__
#define __POTENTIAL__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"

extern void updateDirectPotential(java::ArrayList<Patch *> *scenePatches, Geometry *clusteredWorldGeometry);
extern void updateDirectVisibility(java::ArrayList<Patch *> *scenePatches, Geometry *clusteredWorldGeometry);

#endif
