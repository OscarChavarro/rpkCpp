#ifndef __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS__
#define __VISUAL_DEBUG_TOOLS_GLUT_DEBUG_TOOLS__

#include "galerkin/GalerkinElement.h"
#include "render/opengl/visualDebugTools/GlutDebugToolsModel.h"
#include "scene/Scene.h"

class GlutDebugTools final {
  private:
    GlutDebugToolsModel model;
    int cachedPrimaryPatchIndex;
    int cachedPrimaryHierarchyLevel;
    java::ArrayList<Interaction *> *cachedInteractionsForPrimaryPatch;

    void resizeCallback(int newWidth, int newHeight);
    void keypressCallback(unsigned char keyChar, int x, int y);
    void extendedKeypressCallback(int keyCode, int x, int y);
    void mouseButtonCallback(int button, int state, int x, int y);
    void mouseMotionCallback(int x, int y);
    void drawCallback();
    void printGalerkinElementForPatch(const Scene *scene, int patchIndex);
    void clearCachedPrimaryPatchInteractions();
    void updateCachedPrimaryPatchInteractions(int selectedPatchIndex, int selectedHierarchyLevel);
    static void addInteractionIfNotPresent(
        java::ArrayList<Interaction *> *interactions,
        Interaction *interaction);
    java::ArrayList<Interaction *> *getInteractionsWherePatchParticipateAsSourceOrAsReceiver(
        const Patch *patch,
        int selectedHierarchyLevel) const;

    static GlutDebugTools *&activeGlutDebugToolsInstance();
    static void resizeCallbackBridge(int newWidth, int newHeight);
    static void keypressCallbackBridge(unsigned char keyChar, int x, int y);
    static void extendedKeypressCallbackBridge(int keyCode, int x, int y);
    static void mouseButtonCallbackBridge(int button, int state, int x, int y);
    static void mouseMotionCallbackBridge(int x, int y);
    static void drawCallbackBridge();
    static void printGalerkinElementForPatchBridge(const Scene *scene, int patchIndex);

    void syncModelWindowSizeFromGlut();
    void syncCameraToViewport() const;
    void printElementHierarchy(const GalerkinElement *element, int level);

public:
    explicit GlutDebugTools(const GlutDebugToolsModel &initialModel);
    ~GlutDebugTools();

    void executeGlutGui(int argc, char *argv[]);
};

#endif
