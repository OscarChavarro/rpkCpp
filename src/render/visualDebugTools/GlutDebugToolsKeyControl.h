#ifndef __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS_KEY_CONTROL__
#define __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS_KEY_CONTROL__

#include "render/visualDebugTools/GlutDebugToolsModel.h"

class Scene;

class GlutDebugToolsKeyControl final {
  public:
    static bool handleKeypress(
        unsigned char keyChar,
        GlutDebugToolsModel &model,
        void (*printGalerkinElementForPatch)(const Scene *scene, int patchIndex));

    static bool handleExtendedKeypress(int keyCode, GlutDebugToolsModel &model);

  private:
    static void printSelectedPatchState();
    static int selectedPatchMaxHierarchyLevel(const GlutDebugToolsModel &model);
    static void clampHierarchyLevel(GlutDebugToolsModel &model);
};

#endif
