#ifndef __SHOOTING__
#define __SHOOTING__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "GALERKIN/GalerkinState.h"
#include "GALERKIN/GalerkinRadianceMethod.h"

extern int doShootingStep(Scene *scene, GalerkinState *galerkinState);

#endif
