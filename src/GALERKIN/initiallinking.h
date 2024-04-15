#ifndef __INITIAL_LINKING__
#define __INITIAL_LINKING__

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
    java::ArrayList<Geometry *> *sceneGeometries);

extern void
createInitialLinkWithTopCluster(
    GalerkinElement *element,
    GalerkinRole role,
    GalerkinState *galerkinState);

#endif
