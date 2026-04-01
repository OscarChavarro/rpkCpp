#ifndef __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS_MODEL__
#define __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS_MODEL__

#include "render/opengl/visualDebugTools/GlutDebugMode.h"

class Scene;
class RadianceMethod;
class RenderOptions;
class ParseSession;
class GlutDebugState;

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
    GlutDebugState *debugState;
    void (*memoryFreeCallBack)(ParseSession *mgfContext);
    ParseSession *mgfContext;

    GlutDebugToolsModel();
};

#endif
