#ifndef __SHOOTING__
#define __SHOOTING__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "GALERKIN/GalerkinState.h"

extern int doShootingStep(java::ArrayList<Patch *> *scenePatches, GalerkinState *galerkinState);

#endif
