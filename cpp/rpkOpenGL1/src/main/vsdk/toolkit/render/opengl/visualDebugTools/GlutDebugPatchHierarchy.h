#ifndef __GLUT_DEBUG_PATCH_HIERARCHY__
#define __GLUT_DEBUG_PATCH_HIERARCHY__

#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#include "vsdk/toolkit/scene/Scene.h"

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
    static constexpr double GRAY_DARKEN_FACTOR = 0.42;
    static constexpr double GRAY_CONTRAST_GAMMA = 1.20;
    static constexpr double OUTLINE_MIN_GRAY = 0.05;
    static constexpr double OUTLINE_FROM_SURFACE_FACTOR = 0.65;

    static double clamp01(double value);
    static double toneMappedGrayAndDarkened(double value01);
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
        const Vector3Dd &center,
        double radius,
        int sides,
        const Vector3Dd &axisU,
        const Vector3Dd &axisV,
        const RendererConfiguration *renderOptions);
    static void drawGradientLine(
        const Vector3Dd &start,
        const Vector3Dd &end);
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
