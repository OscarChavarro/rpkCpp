#ifndef __INITIAL_LINKING__
#define __INITIAL_LINKING__

#include "scene/VoxelGrid.h"
#include "GALERKIN/GalerkinElement.h"

enum GalerkinRole {
    SOURCE,
    RECEIVER
};

extern void
createInitialLinks(
    VoxelGrid *sceneWorldVoxelGrid,
    GalerkinElement *top,
    GalerkinRole role,
    GalerkinState *galerkinState,
    java::ArrayList<Geometry *> *sceneGeometries,
    java::ArrayList<Geometry *> *sceneClusteredGeometries);

#endif
