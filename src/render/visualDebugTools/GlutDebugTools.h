#ifndef __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS__
#define __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS__

#include "java/util/ArrayList.h"
#include "scene/RadianceMethod.h"
#include "scene/Scene.h"
#include "io/context/ParseSession.h"
#include "render/visualDebugTools/GlutDebugState.h"

class GalerkinElement;

class GlutDebugTools final {
  public:
    static void executeGlutGui(
        int argc,
        char *argv[],
        Scene *scene,
        RadianceMethod *radianceMethod,
        RenderOptions *renderOptions,
        void (*memoryFreeCallBack)(ParseSession *mgfContext),
        ParseSession *mgfContext);

  private:
    static void resizeCallback(int newWidth, int newHeight);
    static void printElementHierarchy(const GalerkinElement *element, int level);
    static void printGalerkinElementForPatch(const Scene *scene, int patchIndex);
    static void keypressCallback(unsigned char keyChar, int x, int y);
    static void extendedKeypressCallback(int keyCode, int x, int y);
    static void drawCallback();
};

#endif
