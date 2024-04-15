#ifndef __SHOOTING__
#define __SHOOTING__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "GALERKIN//GalerkinState.h"
#include "GALERKIN/GalerkinRadianceMethod.h"

extern int
doShootingStep(
    VoxelGrid *sceneWorldVoxelGrid,
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Geometry *> *sceneGeometries,
    Geometry *clusteredWorldGeometry,
    GalerkinState *galerkinState,
    GalerkinRadianceMethod *galerkinRadianceMethod);

#endif
