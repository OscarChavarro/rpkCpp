#ifndef __LINKING_SIMPLE_STRATEGY__
#define __LINKING_SIMPLE_STRATEGY__

#include "java/util/ArrayList.h"
#include "skin/Geometry.h"
#include "scene/VoxelGrid.h"
#include "GALERKIN/GalerkinRole.h"
#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/GalerkinState.h"
#include "scene/Scene.h"

class LinkingSimpleStrategy {
  public:
    static void
    createInitialLinks(
        const Scene *scene,
        GalerkinElement *topElement,
        GalerkinRole role,
        const GalerkinState *galerkinState);
};

#endif
