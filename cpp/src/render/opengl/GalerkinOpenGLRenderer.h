#ifndef __GALERKIN_OPENGL_RENDERER__
#define __GALERKIN_OPENGL_RENDERER__

#include "common/RenderOptions.h"
#include "galerkin/GalerkinElement.h"
#include "render/opengl/visualDebugTools/GlutDebugState.h"
#include "render/sgl/SglContext.h"
#include "scene/Scene.h"

class GalerkinOpenGLRenderer {
  public:
    static void galerkinRenderPatch(const Patch *patch, const Camera *camera, const RenderOptions *renderOptions);
    static void renderElementHierarchy(const GalerkinElement *element, const RenderOptions *renderOptions);
    static void drawElement(const GalerkinElement *element, int mode, const RenderOptions *renderOptions);
    static void renderScene(const Scene *scene, const RenderOptions *renderOptions, const GlutDebugState *debugState = nullptr);
};

#endif
