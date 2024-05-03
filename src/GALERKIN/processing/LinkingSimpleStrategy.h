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
  private:
    static void
    createInitialLink(
        const Scene *scene,
        const GalerkinState *galerkinState,
        GalerkinRole role,
        java::ArrayList<Geometry *> **candidateList,
        GalerkinElement *topElement,
        BoundingBox *topLevelBoundingBox,
        Patch *patch);

    static void
    geometryLink(
        const Scene *scene,
        const GalerkinState *galerkinState,
        GalerkinRole role,
        java::ArrayList<Geometry *> **candidateList,
        GalerkinElement *topElement,
        BoundingBox *topLevelBoundingBox,
        Geometry *geometry);
  public:
    static void
    createInitialLinks(
        const Scene *scene,
        const GalerkinState *galerkinState,
        GalerkinRole role,
        GalerkinElement *topElement);
};

#endif
