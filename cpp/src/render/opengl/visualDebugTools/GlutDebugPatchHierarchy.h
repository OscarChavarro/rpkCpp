#ifndef __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_PATCH_HIERARCHY__
#define __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_PATCH_HIERARCHY__

#include "common/RenderOptions.h"
#include "galerkin/GalerkinElement.h"
#include "scene/Scene.h"

class GlutDebugPatchHierarchy final {
  public:
    static int maxLevelForSelectedPatch(const Scene *scene, int patchIndex);

    static void renderSelectedPatchAtLevel(
        const Scene *scene,
        const RenderOptions *renderOptions,
        int primaryPatchIndex,
        int secondaryPatchIndex,
        int hierarchyLevel);
    static void renderSecondarySelectedPatchMarker(
        const Scene *scene,
        const RenderOptions *renderOptions,
        int secondaryPatchIndex);

  private:
    static constexpr float GRAY_DARKEN_FACTOR = 0.42f;
    static constexpr float GRAY_CONTRAST_GAMMA = 1.20f;
    static constexpr float OUTLINE_MIN_GRAY = 0.05f;
    static constexpr float OUTLINE_FROM_SURFACE_FACTOR = 0.65f;

    static float clamp01(float value);
    static float toneMappedGrayAndDarkened(float value01);
    static int clampLevel(int level, int maxLevel);
    static void renderNonSelectedPatchesGray(
        const Scene *scene,
        const RenderOptions *renderOptions,
        int primaryPatchIndex,
        int secondaryPatchIndex);
    static void renderElementGray(const GalerkinElement *element, const RenderOptions *renderOptions);
    static const GalerkinElement *selectedPatchRoot(const Scene *scene, int patchIndex);
    static int maxLevelFromElement(const GalerkinElement *element);
    static void renderElementAtLevel(const GalerkinElement *element, int hierarchyLevel, const RenderOptions *renderOptions);
    static void drawSelectedPatchCenterMarker(const GalerkinElement *topLevelElement, const RenderOptions *renderOptions);
    static void drawSecondarySelectedPatchMarker(
        const GalerkinElement *topLevelElement,
        const RenderOptions *renderOptions);
};

#endif
