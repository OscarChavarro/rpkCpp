#ifndef __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS__
#define __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS__

#include "galerkin/GalerkinElement.h"
#include "render/opengl/visualDebugTools/GlutDebugToolsModel.h"
#include "scene/Scene.h"

class GlutDebugTools final {
  public:
    explicit GlutDebugTools(const GlutDebugToolsModel &initialModel);
    ~GlutDebugTools() = default;

    void executeGlutGui(int argc, char *argv[]);

  private:
    GlutDebugToolsModel model;

    void resizeCallback(int newWidth, int newHeight);
    void keypressCallback(unsigned char keyChar, int x, int y);
    void extendedKeypressCallback(int keyCode, int x, int y);
    void mouseButtonCallback(int button, int state, int x, int y);
    void mouseMotionCallback(int x, int y);
    void drawCallback();
    void printGalerkinElementForPatch(const Scene *scene, int patchIndex);

    static GlutDebugTools *&activeGlutDebugToolsInstance();
    static void resizeCallbackBridge(int newWidth, int newHeight);
    static void keypressCallbackBridge(unsigned char keyChar, int x, int y);
    static void extendedKeypressCallbackBridge(int keyCode, int x, int y);
    static void mouseButtonCallbackBridge(int button, int state, int x, int y);
    static void mouseMotionCallbackBridge(int x, int y);
    static void drawCallbackBridge();
    static void printGalerkinElementForPatchBridge(const Scene *scene, int patchIndex);

    void syncModelWindowSizeFromGlut();
    void syncCameraToViewport();
    void printElementHierarchy(const GalerkinElement *element, int level);
};

#endif
