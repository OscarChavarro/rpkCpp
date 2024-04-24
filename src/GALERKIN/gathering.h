#ifndef __GATHERING__
#define __GATHERING__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "GALERKIN/GalerkinState.h"
#include "GALERKIN/GalerkinRadianceMethod.h"

extern float
gatheringPushPullPotential(
    GalerkinElement *element,
    float down);

extern int galerkinRadiosityDoGatheringIteration(Scene *scene, GalerkinState *galerkinState);
extern int doClusteredGatheringIteration(Scene *scene, GalerkinState *galerkinState);

#endif
