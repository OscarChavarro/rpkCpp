#ifndef __GATHERING__
#define __GATHERING__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"

extern int galerkinRadiosityDoGatheringIteration(java::ArrayList<Patch *> *scenePatches);
extern int doClusteredGatheringIteration(java::ArrayList<Patch *> *scenePatches);

#endif
