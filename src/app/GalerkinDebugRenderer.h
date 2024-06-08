#ifndef __GALERKIN_DEBUG_RENDERER__
#define __GALERKIN_DEBUG_RENDERER__

#include "scene/Scene.h"
#include "GALERKIN/GalerkinElement.h"

class GalerkinDebugRenderer {
  private:
    static void recursiveDrawElement(const GalerkinElement *element, GalerkinElementRenderMode mode, const RenderOptions *renderOptions);

  public:
    static void renderGalerkinElementHierarchy(const Scene *scene, const RenderOptions *renderOptions);
};

#endif
