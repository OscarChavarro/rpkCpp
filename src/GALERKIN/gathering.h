#ifndef __GATHERING__
#define __GATHERING__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "GALERKIN/GalerkinState.h"
#include "GALERKIN/GalerkinRadianceMethod.h"

extern int galerkinRadiosityDoGatheringIteration(java::ArrayList<Patch *> *scenePatches, GalerkinState *galerkinState, GalerkinRadianceMethod *galerkinRadianceMethod);
extern int doClusteredGatheringIteration(java::ArrayList<Patch *> *scenePatches, GalerkinState *galerkinState, GalerkinRadianceMethod *galerkinRadianceMethod);

#endif
