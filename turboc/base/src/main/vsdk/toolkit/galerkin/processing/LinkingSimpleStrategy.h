#ifndef __LINKING_SIMPLE_STRATEGY__
#define __LINKING_SIMPLE_STRATEGY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#include "vsdk/toolkit/galerkin/GalerkinRole.h"
#include "vsdk/toolkit/galerkin/GalerkinState.h"

class LinkingSimpleStrategy {
  private:
    static void
    createInitialLink(
        const Scene *scene,
        const GalerkinState *galerkinState,
        GalerkinRole role,
        ArrayList<Geometry *> **candidateList,
        GalerkinElement *topElement,
        BoundingBox *topLevelBoundingBox,
        Patch *patch);

    static void
    geometryLink(
        const Scene *scene,
        const GalerkinState *galerkinState,
        GalerkinRole role,
        ArrayList<Geometry *> **candidateList,
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
