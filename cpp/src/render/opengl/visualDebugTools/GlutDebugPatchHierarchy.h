#ifndef __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_PATCH_HIERARCHY__
#define __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_PATCH_HIERARCHY__

#include "material/RendererConfiguration.h"
#include "galerkin/GalerkinElement.h"
#include "scene/Scene.h"

class GlutDebugPatchHierarchy final {
  public:
    static int maxLevelForSelectedPatch(const Scene *scene, int patchIndex);
    static int maxLevelAcrossScene(const Scene *scene);

    static void renderSelectedPatchAtLevel(
        const Scene *scene,
        const RendererConfiguration *renderOptions,
        int primaryPatchIndex,
        int secondaryPatchIndex,
        int hierarchyLevel,
        const java::ArrayList<Interaction *> *interactionsToRender);
    static void renderInteractionBetweenSelected(
        const Scene *scene,
        int primaryPatchIndex,
        int secondaryPatchIndex,
        const java::ArrayList<Interaction *> *interactionsToRender);
    static void renderInteractingPatchesAtLevelIfNoSecondary(
        const Scene *scene,
        const RendererConfiguration *renderOptions,
        int primaryPatchIndex,
        int secondaryPatchIndex,
        int hierarchyLevel,
        const java::ArrayList<Interaction *> *interactionsToRender);
    static void renderSecondarySelectedPatchMarker(
        const Scene *scene,
        const RendererConfiguration *renderOptions,
        int secondaryPatchIndex,
        int hierarchyLevel);

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
        const RendererConfiguration *renderOptions,
        int primaryPatchIndex,
        int secondaryPatchIndex,
        const java::ArrayList<Interaction *> *interactionsToRender);
    static void renderElementGray(const GalerkinElement *element, const RendererConfiguration *renderOptions);
    static const GalerkinElement *selectedPatchRoot(const Scene *scene, int patchIndex);
    static int maxLevelFromElement(const GalerkinElement *element);
    static void renderElementAtLevel(const GalerkinElement *element, int hierarchyLevel, const RendererConfiguration *renderOptions);
    static void drawCenterMark(
        const Vector3D &center,
        float radius,
        int sides,
        const Vector3D &axisU,
        const Vector3D &axisV,
        const RendererConfiguration *renderOptions);
    static void drawGradientLine(
        const Vector3D &start,
        const Vector3D &end);
    static void drawSelectedPatchCenterMarker(const GalerkinElement *topLevelElement, const RendererConfiguration *renderOptions);
    static void drawInteractions(const java::ArrayList<Interaction *> *interactionsToRender);
    static void addPatchIfNotPresent(
        java::ArrayList<const Patch *> *patches,
        const Patch *patch);
    static void drawSecondarySelectedPatchMarker(
        const GalerkinElement *topLevelElement,
        const RendererConfiguration *renderOptions,
        int hierarchyLevel);
};

#endif
