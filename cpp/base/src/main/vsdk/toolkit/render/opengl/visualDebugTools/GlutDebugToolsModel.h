#ifndef VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS_MODEL__
#define VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS_MODEL__

#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"
#include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugMode.h"
#include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugState.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

class GlutDebugToolsModel {
  public:
    GlutDebugMode mode;
    bool fullScreen;
    bool fullScreenApplied;
    int selectedHierarchyLevel;
    int width;
    int height;
    int windowedWidth;
    int windowedHeight;
    Scene *scene;
    RadianceMethod *radianceMethod;
    RendererConfiguration *renderOptions;
    ToneMappingContext *toneMapOptions;
    GlutDebugState *debugState;
    void (*memoryFreeCallBack)(ParseRuntimeContext *mgfContext);
    ParseRuntimeContext *mgfContext;

    GlutDebugToolsModel();
};

#endif
