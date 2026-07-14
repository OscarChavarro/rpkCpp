#ifndef VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS_KEY_CONTROL__
#define VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS_KEY_CONTROL__

#include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugToolsModel.h"
#include "vsdk/toolkit/scene/Scene.h"

class GlutDebugToolsKeyControl final {
  public:
    static bool handleKeypress(
        unsigned char keyChar,
        GlutDebugToolsModel &model,
        void (*printGalerkinElementForPatch)(const Scene *scene, int patchIndex));

    static bool handleExtendedKeypress(int keyCode, GlutDebugToolsModel &model);

  private:
    static bool isGalerkinPatchIndex(const Scene *scene, int patchIndex);
    static int selectedPatchMaxHierarchyLevel(const GlutDebugToolsModel &model);
    static void clampHierarchyLevel(GlutDebugToolsModel &model);
    static void stepSelectedPatchIndex(int *selectedPatchIndex, int delta, const Scene *scene);
};

#endif
