#ifndef GALERKIN_OPENGL_RENDERER__
#define GALERKIN_OPENGL_RENDERER__

#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugState.h"
#include "vsdk/toolkit/render/sgl/SglContext.h"
#include "vsdk/toolkit/scene/Scene.h"

class GalerkinOpenGLRenderer {
  public:
    static void galerkinRenderPatch(const Patch *patch, const Camera *camera, const RendererConfiguration *renderOptions);
    static void renderElementHierarchy(const GalerkinElement *element, const RendererConfiguration *renderOptions);
    static void drawElement(const GalerkinElement *element, int mode, const RendererConfiguration *renderOptions);
    static void renderScene(const Scene *scene, const RendererConfiguration *renderOptions, const GlutDebugState *debugState = nullptr);
};

#endif
