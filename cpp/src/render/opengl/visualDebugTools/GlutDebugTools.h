#ifndef VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS__
#define VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS__

#include "galerkin/GalerkinElement.h"
#include "render/opengl/visualDebugTools/GlutDebugToolsModel.h"
#include "scene/Scene.h"

class GlutDebugTools final {
  private:
    GlutDebugToolsModel model;
    int cachedPrimaryPatchIndex;
    int cachedSecondaryPatchIndex;
    int cachedPrimaryHierarchyLevel;
    bool forcePrimaryPatchInteractionsRefresh;
    java::ArrayList<Interaction *> *cachedInteractionsForPrimaryPatch;

    void resizeCallback(int newWidth, int newHeight);
    void keypressCallback(unsigned char keyChar, int x, int y);
    void extendedKeypressCallback(int keyCode, int x, int y);
    void mouseButtonCallback(int button, int state, int x, int y);
    void mouseMotionCallback(int x, int y);
    void drawCallback();
    void printGalerkinElementForPatch(const Scene *scene, int patchIndex);
    void clearCachedPrimaryPatchInteractions();
    void updateCachedPrimaryPatchInteractions(
        int selectedPatchIndex,
        int secondarySelectedPatchIndex,
        int selectedHierarchyLevel);
    static void addInteractionIfNotPresent(
        java::ArrayList<Interaction *> *interactions,
        Interaction *interaction);
    java::ArrayList<Interaction *> *getInteractionsWherePatchParticipateAsSourceOrAsReceiver(
        const Patch *patch,
        const Patch *secondaryPatch,
        int selectedHierarchyLevel) const;

    static GlutDebugTools *&activeGlutDebugToolsInstance();
    static void resizeCallbackBridge(int newWidth, int newHeight);
    static void keypressCallbackBridge(unsigned char keyChar, int x, int y);
    static void extendedKeypressCallbackBridge(int keyCode, int x, int y);
    static void mouseButtonCallbackBridge(int button, int state, int x, int y);
    static void mouseMotionCallbackBridge(int x, int y);
    static void drawCallbackBridge();
    static void printGalerkinElementForPatchBridge(const Scene *scene, int patchIndex);
    static int maxHierarchyLevelFromElement(const GalerkinElement *element);
    static bool isElementInHierarchy(
        const GalerkinElement *hierarchyRoot,
        const GalerkinElement *candidateElement);
    static void addInteractionsFromElementLevel(
        const GalerkinElement *element,
        int hierarchyLevel,
        java::ArrayList<Interaction *> *interactions);

    void syncModelWindowSizeFromGlut();
    void syncCameraToViewport() const;
    void printElementHierarchy(const GalerkinElement *element, int level);

public:
    explicit GlutDebugTools(const GlutDebugToolsModel &initialModel);
    ~GlutDebugTools();

    void executeGlutGui(int argc, char *argv[]);
};

#endif
