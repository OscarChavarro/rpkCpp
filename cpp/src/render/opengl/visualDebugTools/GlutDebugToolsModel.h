#ifndef __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS_MODEL__
#define __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS_MODEL__

#include "common/RenderOptions.h"
#include "io/context/ParseRuntimeContext.h"
#include "render/opengl/visualDebugTools/GlutDebugMode.h"
#include "render/opengl/visualDebugTools/GlutDebugState.h"
#include "scene/RadianceMethod.h"
#include "scene/Scene.h"
#include "tonemap/ToneMappingContext.h"

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
    RenderOptions *renderOptions;
    ToneMappingContext *toneMapOptions;
    GlutDebugState *debugState;
    void (*memoryFreeCallBack)(ParseRuntimeContext *mgfContext);
    ParseRuntimeContext *mgfContext;

    GlutDebugToolsModel();
};

#endif
