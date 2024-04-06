#ifndef __SHOOTING__
#define __SHOOTING__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"

extern int doShootingStep(java::ArrayList<Patch *> *scenePatches, GalerkinState *galerkinState);

#endif
