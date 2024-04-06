/**
All kind of form factor computations
*/

#ifndef __FORM_FACTOR__
#define __FORM_FACTOR__

#include "java/util/ArrayList.h"
#include "GALERKIN/Interaction.h"
#include "GALERKIN/GalerkinState.h"

extern int facing(Patch *P, Patch *Q);
extern unsigned areaToAreaFormFactor(
    Interaction *link,
    java::ArrayList<Geometry *> *geometryShadowList,
    bool isSceneGeometry,
    bool isClusteredGeometry,
    GalerkinState *galerkinState);

#endif
