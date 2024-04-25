#ifndef __GATHERING__
#define __GATHERING__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "GALERKIN/GalerkinState.h"
#include "GALERKIN/GalerkinRadianceMethod.h"

extern float gatheringPushPullPotential(GalerkinElement *element, float down);
extern int galerkinDoGatheringIteration(Scene *scene, GalerkinState *galerkinState, RenderOptions *renderOptions);
extern int galerkinDoClusteredGatheringIteration(Scene *scene, GalerkinState *galerkinState, RenderOptions *renderOptions);

#endif
