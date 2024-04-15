#ifndef __SHOOTING__
#define __SHOOTING__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "GALERKIN//GalerkinState.h"
#include "GALERKIN/GalerkinRadianceMethod.h"

extern int
doShootingStep(
    Camera *camera,
    VoxelGrid *sceneWorldVoxelGrid,
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Geometry *> *sceneGeometries,
    java::ArrayList<Geometry *> *sceneClusteredGeometries,
    Geometry *clusteredWorldGeometry,
    GalerkinState *galerkinState);

#endif
