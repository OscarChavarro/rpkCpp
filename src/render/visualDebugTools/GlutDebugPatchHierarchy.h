#ifndef __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_PATCH_HIERARCHY__
#define __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_PATCH_HIERARCHY__

class Scene;
class RenderOptions;
class GalerkinElement;

class GlutDebugPatchHierarchy final {
  public:
    static int maxLevelForSelectedPatch(const Scene *scene, int patchIndex);

    static void renderSelectedPatchAtLevel(
        const Scene *scene,
        const RenderOptions *renderOptions,
        int patchIndex,
        int hierarchyLevel);

  private:
    static int clampLevel(int level, int maxLevel);
    static void renderNonSelectedPatchesGray(const Scene *scene, const RenderOptions *renderOptions, int selectedPatchIndex);
    static void renderElementGray(const GalerkinElement *element, const RenderOptions *renderOptions);
    static const GalerkinElement *selectedPatchRoot(const Scene *scene, int patchIndex);
    static int maxLevelFromElement(const GalerkinElement *element);
    static void renderElementAtLevel(const GalerkinElement *element, int hierarchyLevel, const RenderOptions *renderOptions);
    static void drawSelectedPatchCenterMarker(const GalerkinElement *topLevelElement, const RenderOptions *renderOptions);
};

#endif
