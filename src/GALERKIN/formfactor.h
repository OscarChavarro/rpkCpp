/**
All kind of form factor computations
*/

#ifndef __FORM_FACTOR__
#define __FORM_FACTOR__

#include "java/util/ArrayList.h"
#include "scene/VoxelGrid.h"
#include "GALERKIN/Interaction.h"

extern unsigned
areaToAreaFormFactor(
    VoxelGrid *sceneWorldVoxelGrid,
    Interaction *link,
    java::ArrayList<Geometry *> *geometryShadowList,
    bool isSceneGeometry,
    bool isClusteredGeometry,
    GalerkinState *galerkinState);

extern bool
facing(Patch *P, Patch *Q);

#endif
