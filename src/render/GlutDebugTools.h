#ifndef __GLUT__
#define __GLUT__

#include "java/util/ArrayList.h"
#include "scene/RadianceMethod.h"
#include "scene/Scene.h"
#include "io/context/MgfParseSession.h"
#include "render/GlutDebugState.h"

class GalerkinElement;

class GlutDebugTools final {
  public:
    static void executeGlutGui(
        int argc,
        char *argv[],
        Scene *scene,
        RadianceMethod *radianceMethod,
        RenderOptions *renderOptions,
        void (*memoryFreeCallBack)(MgfParseSession *mgfContext),
        MgfParseSession *mgfContext);

  private:
    static void resizeCallback(int newWidth, int newHeight);
    static void printElementHierarchy(const GalerkinElement *element, int level);
    static void printGalerkinElementForPatch(const Scene *scene, int patchIndex);
    static void keypressCallback(unsigned char keyChar, int x, int y);
    static void extendedKeypressCallback(int keyCode, int x, int y);
    static void drawCallback();
};

#endif
