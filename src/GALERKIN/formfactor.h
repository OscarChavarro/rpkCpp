/**
All kind of form factor computations
*/

#ifndef __FORM_FACTOR__
#define __FORM_FACTOR__

#include "java/util/ArrayList.h"
#include "scene/VoxelGrid.h"
#include "GALERKIN/Interaction.h"

extern void
computeAreaToAreaFormFactorVisibility(
    const VoxelGrid *sceneWorldVoxelGrid,
    Interaction *link,
    const java::ArrayList<Geometry *> *geometryShadowList,
    const bool isSceneGeometry,
    const bool isClusteredGeometry,
    GalerkinState *galerkinState);

#endif
