#ifndef __LINKING_SIMPLE_STRATEGY__
#define __LINKING_SIMPLE_STRATEGY__

#include "java/util/ArrayList.h"
#include "skin/Geometry.h"
#include "scene/VoxelGrid.h"
#include "GALERKIN/GalerkinRole.h"
#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/GalerkinState.h"

class LinkingSimpleStrategy {
  public:
    static void
    createInitialLinks(
        VoxelGrid *sceneWorldVoxelGrid,
        GalerkinElement *top,
        GalerkinRole role,
        GalerkinState *galerkinState,
        java::ArrayList<Geometry *> *sceneGeometries,
        java::ArrayList<Geometry *> *sceneClusteredGeometries);
};

#endif
