#ifndef __GALERKIN_OPENGL_RENDERER__
#define __GALERKIN_OPENGL_RENDERER__

#include "material/RendererConfiguration.h"
#include "galerkin/GalerkinElement.h"
#include "render/opengl/visualDebugTools/GlutDebugState.h"
#include "render/sgl/SglContext.h"
#include "scene/Scene.h"

class GalerkinOpenGLRenderer {
  public:
    static void galerkinRenderPatch(const Patch *patch, const Camera *camera, const RendererConfiguration *renderOptions);
    static void renderElementHierarchy(const GalerkinElement *element, const RendererConfiguration *renderOptions);
    static void drawElement(const GalerkinElement *element, int mode, const RendererConfiguration *renderOptions);
    static void renderScene(const Scene *scene, const RendererConfiguration *renderOptions, const GlutDebugState *debugState = nullptr);
};

#endif
