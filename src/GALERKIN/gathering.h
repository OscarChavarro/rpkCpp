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

extern int
galerkinRadiosityDoGatheringIteration(
    Camera *camera,
    VoxelGrid *sceneWorldVoxelGrid,
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Geometry *> *sceneGeometries,
    java::ArrayList<Geometry *> *sceneClusteredGeometries,
    Geometry *clusteredWorldGeometry,
    GalerkinState *galerkinState);

extern int
doClusteredGatheringIteration(
    Camera *camera,
    VoxelGrid *sceneWorldVoxelGrid,
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Geometry *> *sceneGeometries,
    java::ArrayList<Geometry *> *sceneClusteredGeometries,
    Geometry *clusteredWorldGeometry,
    GalerkinState *galerkinState);

#endif
