#include "render/opengl/visualDebugTools/GlutDebugTools.h"
#include "render/opengl/visualDebugTools/GlutDebugState.h"
#include "render/opengl/visualDebugTools/GlutDebugToolsKeyControl.h"
#include "render/opengl/visualDebugTools/GlutDebugToolsMouseControl.h"
#include "render/opengl/visualDebugTools/GlutDebugPatchHierarchy.h"
#include "render/opengl/visualDebugTools/GlutHudConsole.h"
#include "galerkin/GalerkinElement.h"

#ifdef OPEN_GL_ENABLED

#ifdef __APPLE__
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

#include <cstdio>

#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "render/opengl/Opengl.h"

namespace {
void
addInteractionIfNotPresentLocal(
    java::ArrayList<Interaction *> *interactions,
    Interaction *interaction)
{
    if ( interactions == nullptr || interaction == nullptr ) {
        return;
    }

    for ( int i = 0; i < interactions->size(); i++ ) {
        if ( interactions->get(i) == interaction ) {
            return;
        }
    }

    interactions->add(interaction);
}

int
maxHierarchyLevelFromElement(const GalerkinElement *element) {
    if ( element == nullptr || element->regularSubElements == nullptr ) {
        return 0;
    }

    int maxDepth = 0;
    for ( int i = 0; i < 4; i++ ) {
        const GalerkinElement *child = static_cast<const GalerkinElement *>(element->regularSubElements[i]);
        if ( child == nullptr ) {
            continue;
        }
        const int childDepth = 1 + maxHierarchyLevelFromElement(child);
        if ( childDepth > maxDepth ) {
            maxDepth = childDepth;
        }
    }
    return maxDepth;
}

bool
isElementInHierarchy(
    const GalerkinElement *hierarchyRoot,
    const GalerkinElement *candidateElement)
{
    if ( hierarchyRoot == nullptr || candidateElement == nullptr ) {
        return false;
    }
    if ( hierarchyRoot == candidateElement ) {
        return true;
    }
    if ( hierarchyRoot->regularSubElements == nullptr ) {
        return false;
    }

    for ( int i = 0; i < 4; i++ ) {
        const GalerkinElement *child = static_cast<const GalerkinElement *>(hierarchyRoot->regularSubElements[i]);
        if ( child != nullptr && isElementInHierarchy(child, candidateElement) ) {
            return true;
        }
    }
    return false;
}

void
addInteractionsFromElementLevel(
    const GalerkinElement *element,
    int hierarchyLevel,
    java::ArrayList<Interaction *> *interactions)
{
    if ( element == nullptr || interactions == nullptr ) {
        return;
    }

    if ( hierarchyLevel <= 0 || element->regularSubElements == nullptr ) {
        for ( int i = 0; element->interactions != nullptr && i < element->interactions->size(); i++ ) {
            Interaction *interaction = element->interactions->get(i);
            if ( interaction != nullptr && interaction->receiverElement == element ) {
                addInteractionIfNotPresentLocal(interactions, interaction);
            }
        }
        return;
    }

    for ( int i = 0; i < 4; i++ ) {
        const GalerkinElement *child = static_cast<const GalerkinElement *>(element->regularSubElements[i]);
        if ( child != nullptr ) {
            addInteractionsFromElementLevel(child, hierarchyLevel - 1, interactions);
        }
    }
}
}

GlutDebugTools *&
GlutDebugTools::activeGlutDebugToolsInstance() {
    static GlutDebugTools *activeInstance = nullptr;
    return activeInstance;
}

void
GlutDebugTools::resizeCallbackBridge(int newWidth, int newHeight) {
    GlutDebugTools * const activeInstance = activeGlutDebugToolsInstance();
    if ( activeInstance != nullptr ) {
        activeInstance->resizeCallback(newWidth, newHeight);
    }
}

void
GlutDebugTools::keypressCallbackBridge(unsigned char keyChar, int x, int y) {
    GlutDebugTools * const activeInstance = activeGlutDebugToolsInstance();
    if ( activeInstance != nullptr ) {
        activeInstance->keypressCallback(keyChar, x, y);
    }
}

void
GlutDebugTools::extendedKeypressCallbackBridge(int keyCode, int x, int y) {
    GlutDebugTools * const activeInstance = activeGlutDebugToolsInstance();
    if ( activeInstance != nullptr ) {
        activeInstance->extendedKeypressCallback(keyCode, x, y);
    }
}

void
GlutDebugTools::mouseButtonCallbackBridge(int button, int state, int x, int y) {
    GlutDebugTools * const activeInstance = activeGlutDebugToolsInstance();
    if ( activeInstance != nullptr ) {
        activeInstance->mouseButtonCallback(button, state, x, y);
    }
}

void
GlutDebugTools::mouseMotionCallbackBridge(int x, int y) {
    GlutDebugTools * const activeInstance = activeGlutDebugToolsInstance();
    if ( activeInstance != nullptr ) {
        activeInstance->mouseMotionCallback(x, y);
    }
}

void
GlutDebugTools::drawCallbackBridge() {
    GlutDebugTools * const activeInstance = activeGlutDebugToolsInstance();
    if ( activeInstance != nullptr ) {
        activeInstance->drawCallback();
    }
}

void
GlutDebugTools::printGalerkinElementForPatchBridge(const Scene *scene, int patchIndex) {
    GlutDebugTools * const activeInstance = activeGlutDebugToolsInstance();
    if ( activeInstance != nullptr ) {
        activeInstance->printGalerkinElementForPatch(scene, patchIndex);
    }
}

GlutDebugTools::GlutDebugTools(const GlutDebugToolsModel &initialModel):
    model(initialModel),
    cachedPrimaryPatchIndex(-2),
    cachedSecondaryPatchIndex(-2),
    cachedPrimaryHierarchyLevel(-1),
    forcePrimaryPatchInteractionsRefresh(false),
    cachedInteractionsForPrimaryPatch(nullptr)
{
    model.selectedHierarchyLevel = GlutDebugPatchHierarchy::maxLevelAcrossScene(model.scene);
}

GlutDebugTools::~GlutDebugTools() {
    clearCachedPrimaryPatchInteractions();
}

void
GlutDebugTools::clearCachedPrimaryPatchInteractions() {
    if ( cachedInteractionsForPrimaryPatch != nullptr ) {
        delete cachedInteractionsForPrimaryPatch;
        cachedInteractionsForPrimaryPatch = nullptr;
    }
}

void
GlutDebugTools::addInteractionIfNotPresent(
    java::ArrayList<Interaction *> *interactions,
    Interaction *interaction)
{
    if ( interactions == nullptr || interaction == nullptr ) {
        return;
    }

    for ( int i = 0; i < interactions->size(); i++ ) {
        if ( interactions->get(i) == interaction ) {
            return;
        }
    }

    interactions->add(interaction);
}

java::ArrayList<Interaction *> *
GlutDebugTools::getInteractionsWherePatchParticipateAsSourceOrAsReceiver(
    const Patch *patch,
    const Patch *secondaryPatch,
    int selectedHierarchyLevel) const
{
    java::ArrayList<Interaction *> *interactions = new java::ArrayList<Interaction *>();
    if ( patch == nullptr ) {
        return interactions;
    }
    if ( patch->getRadianceData() == nullptr || patch->getRadianceData()->className != ElementTypes::ELEMENT_GALERKIN ) {
        return interactions;
    }

    const GalerkinElement *topLevelElement = GalerkinElement::fromPatch(patch);
    if ( topLevelElement == nullptr ) {
        return interactions;
    }

    int clampedLevel = selectedHierarchyLevel;
    if ( clampedLevel < 0 ) {
        clampedLevel = 0;
    }
    const int maxLevel = maxHierarchyLevelFromElement(topLevelElement);
    if ( clampedLevel > maxLevel ) {
        clampedLevel = maxLevel;
    }

    addInteractionsFromElementLevel(topLevelElement, clampedLevel, interactions);
    if ( secondaryPatch == nullptr ) {
        return interactions;
    }
    if ( secondaryPatch->getRadianceData() == nullptr
         || secondaryPatch->getRadianceData()->className != ElementTypes::ELEMENT_GALERKIN ) {
        delete interactions;
        interactions = new java::ArrayList<Interaction *>();
        return interactions;
    }

    const GalerkinElement *secondaryTopLevelElement = GalerkinElement::fromPatch(secondaryPatch);
    if ( secondaryTopLevelElement == nullptr ) {
        delete interactions;
        interactions = new java::ArrayList<Interaction *>();
        return interactions;
    }

    java::ArrayList<Interaction *> *filteredInteractions = new java::ArrayList<Interaction *>();
    for ( int i = 0; i < interactions->size(); i++ ) {
        Interaction *interaction = interactions->get(i);
        if ( interaction == nullptr ) {
            continue;
        }

        if ( isElementInHierarchy(secondaryTopLevelElement, interaction->sourceElement)
             || isElementInHierarchy(secondaryTopLevelElement, interaction->receiverElement) ) {
            addInteractionIfNotPresentLocal(filteredInteractions, interaction);
        }
    }

    delete interactions;
    return filteredInteractions;
}

void
GlutDebugTools::updateCachedPrimaryPatchInteractions(
    int selectedPatchIndex,
    int secondarySelectedPatchIndex,
    int selectedHierarchyLevel)
{
    if ( !forcePrimaryPatchInteractionsRefresh
         && selectedPatchIndex == cachedPrimaryPatchIndex
         && secondarySelectedPatchIndex == cachedSecondaryPatchIndex
         && selectedHierarchyLevel == cachedPrimaryHierarchyLevel ) {
        return;
    }
    forcePrimaryPatchInteractionsRefresh = false;

    cachedPrimaryPatchIndex = selectedPatchIndex;
    cachedSecondaryPatchIndex = secondarySelectedPatchIndex;
    cachedPrimaryHierarchyLevel = selectedHierarchyLevel;
    clearCachedPrimaryPatchInteractions();

    if ( selectedPatchIndex < 0
         || model.scene == nullptr
         || model.scene->patchList == nullptr
         || selectedPatchIndex >= model.scene->patchList->size() ) {
        return;
    }

    const Patch *secondaryPatch = nullptr;
    if ( secondarySelectedPatchIndex >= 0 && secondarySelectedPatchIndex < model.scene->patchList->size() ) {
        secondaryPatch = model.scene->patchList->get(secondarySelectedPatchIndex);
    }

    const Patch *selectedPatch = model.scene->patchList->get(selectedPatchIndex);
    cachedInteractionsForPrimaryPatch = getInteractionsWherePatchParticipateAsSourceOrAsReceiver(
        selectedPatch,
        secondaryPatch,
        selectedHierarchyLevel);
}

void
GlutDebugTools::resizeCallback(const int newWidth, const int newHeight) {
    if ( newWidth <= 0 || newHeight <= 0 ) {
        return;
    }
    model.width = newWidth;
    model.height = newHeight;
    if ( !model.fullScreen ) {
        model.windowedWidth = newWidth;
        model.windowedHeight = newHeight;
    }
}

void
GlutDebugTools::syncModelWindowSizeFromGlut() {
    const int currentWidth = glutGet(GLUT_WINDOW_WIDTH);
    const int currentHeight = glutGet(GLUT_WINDOW_HEIGHT);
    if ( currentWidth <= 0 || currentHeight <= 0 ) {
        return;
    }

    model.width = currentWidth;
    model.height = currentHeight;
    if ( !model.fullScreen ) {
        model.windowedWidth = currentWidth;
        model.windowedHeight = currentHeight;
    }
}

void
GlutDebugTools::syncCameraToViewport() const {
    if ( model.scene == nullptr || model.scene->camera == nullptr ) {
        return;
    }
    if ( model.width <= 0 || model.height <= 0 ) {
        return;
    }

    Camera *camera = model.scene->camera;
    if ( camera->xSize == model.width &&
         camera->ySize == model.height &&
         camera->pixelWidth > Numeric::EPSILON_FLOAT &&
         camera->pixelHeight > Numeric::EPSILON_FLOAT ) {
        return;
    }

    camera->set(
        &camera->eyePosition,
        &camera->lookPosition,
        &camera->upDirection,
        camera->fieldOfVision,
        model.width,
        model.height,
        &camera->background);
}

void
GlutDebugTools::printElementHierarchy(const GalerkinElement *element, int level) {
    switch ( level ) {
        case 0:
            break;
        case 1:
            java::System::out.printf("  - ");
            break;
        case 2:
            java::System::out.printf("    . ");
            break;
        default:
            java::System::out.printf("      (%d) ", level);
            for ( int i = 3; i < level; i++ ) {
                java::System::out.printf(" ");
            }
            java::System::out.printf("-> ");
            break;
    }
    const ColorRgb *c = element->radiance;
    long int numberOfInteractions = 0;
    if ( element->interactions != nullptr ) {
        numberOfInteractions = element->interactions->size();
    }

    if ( element->regularSubElements == nullptr ) {
        if ( c == nullptr ) {
            java::System::out.printf("Child element no radiance\n");
        } else {
            java::System::out.printf("Child element radiance <%0.4f, %0.4f, %0.4f>, interactions: %ld\n",
               c->r, c->g, c->b, numberOfInteractions);
        }
    } else {
        if ( c == nullptr ) {
            java::System::out.printf("Container element no radiance\n");
        } else {
            java::System::out.printf("Container element radiance <%0.4f, %0.4f, %0.4f>, interactions: %ld\n",
               c->r, c->g, c->b, numberOfInteractions);
        }
        for ( int i = 0; i < 4; i++ ) {
            const GalerkinElement *child = static_cast<GalerkinElement *>(element->regularSubElements[i]);
            if ( child != nullptr ) {
                printElementHierarchy(child, level + 1);
            }
        }
    }
}

void
GlutDebugTools::printGalerkinElementForPatch(const Scene *scene, int patchIndex) {
    java::System::out.printf("================================================================================\n");
    if ( patchIndex < 0 || scene->patchList == nullptr || patchIndex >= scene->patchList->size() ) {
        return;
    }
    const Patch *patch = scene->patchList->get(patchIndex);
    if  ( patch == nullptr || patch->getRadianceData() == nullptr ) {
        return;
    }
    const GalerkinElement *element = GalerkinElement::fromPatch(patch);
    java::System::out.printf("Galerkin element for patch[%d] %d\n", patchIndex, patch->getId());
    printElementHierarchy(element, 0);
}

void
GlutDebugTools::keypressCallback(unsigned char keyChar, int /*x*/, int /*y*/) {
    if ( GlutDebugToolsKeyControl::handleKeypress(
             keyChar,
             model,
             printGalerkinElementForPatchBridge) ) {
        if ( keyChar == ' ' ) {
            forcePrimaryPatchInteractionsRefresh = true;
        }
        glutPostRedisplay();
    }
}

void
GlutDebugTools::extendedKeypressCallback(int keyCode, int /*x*/, int /*y*/) {
    if ( GlutDebugToolsKeyControl::handleExtendedKeypress(keyCode, model) ) {
        glutPostRedisplay();
    }
}

void
GlutDebugTools::mouseButtonCallback(int button, int state, int x, int y) {
    syncModelWindowSizeFromGlut();
    syncCameraToViewport();
    if ( GlutDebugToolsMouseControl::handleMouseButton(button, state, x, y, model) ) {
        glutPostRedisplay();
    }
}

void
GlutDebugTools::mouseMotionCallback(int x, int y) {
    syncModelWindowSizeFromGlut();
    syncCameraToViewport();
    if ( GlutDebugToolsMouseControl::handleMouseMotion(x, y, model) ) {
        glutPostRedisplay();
    }
}

void
GlutDebugTools::drawCallback() {
    if ( model.scene == nullptr || model.renderOptions == nullptr ) {
        return;
    }

    syncModelWindowSizeFromGlut();
    syncCameraToViewport();

    if ( model.fullScreen != model.fullScreenApplied ) {
        if ( model.fullScreen ) {
            glutFullScreen();
            model.fullScreenApplied = true;
        } else {
            glutPositionWindow(0, 0);
            glutReshapeWindow(model.windowedWidth, model.windowedHeight);
            model.fullScreenApplied = false;
        }
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, model.width, model.height);

    int totalElements = 0;
    int selectedPatchIndex = -1;
    int secondarySelectedPatchIndex = -1;
    if ( model.debugState != nullptr ) {
        selectedPatchIndex = model.debugState->primarySelectedPatch;
        secondarySelectedPatchIndex = model.debugState->selectedSelectedPatch;
    }
    if ( selectedPatchIndex < -1 ) {
        selectedPatchIndex = -1;
    }
    if ( secondarySelectedPatchIndex < -1 ) {
        secondarySelectedPatchIndex = -1;
    }
    if ( model.scene->patchList != nullptr ) {
        totalElements = model.scene->patchList->size();
        if ( selectedPatchIndex >= totalElements ) {
            selectedPatchIndex = totalElements - 1;
        }
        if ( secondarySelectedPatchIndex >= totalElements ) {
            secondarySelectedPatchIndex = totalElements - 1;
        }
        if ( totalElements <= 0 ) {
            selectedPatchIndex = -1;
            secondarySelectedPatchIndex = -1;
        }
    }
    updateCachedPrimaryPatchInteractions(
        selectedPatchIndex,
        secondarySelectedPatchIndex,
        model.selectedHierarchyLevel);

    if ( model.mode == GlutDebugMode::RADIANCE_SCENE ) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINES);
        Opengl::openGlRenderScene(
            model.scene,
            model.radianceMethod,
            model.toneMapOptions,
            model.renderOptions,
            model.debugState);
    } else if ( model.mode == GlutDebugMode::GALERKIN_ELEMENT_HIERARCHY ) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        Opengl::openGlRenderSetCamera(model.scene->camera, model.scene->geometryList);
        glPushMatrix();
        Opengl::openGlApplyDebugSceneRotation(model.scene, model.debugState);
        GlutDebugPatchHierarchy::renderSelectedPatchAtLevel(
            model.scene,
            model.renderOptions,
            selectedPatchIndex,
            secondarySelectedPatchIndex,
            model.selectedHierarchyLevel,
            cachedInteractionsForPrimaryPatch);
        GlutDebugPatchHierarchy::renderSecondarySelectedPatchMarker(
            model.scene,
            model.renderOptions,
            secondarySelectedPatchIndex,
            model.selectedHierarchyLevel);
        GlutDebugPatchHierarchy::renderInteractingPatchesAtLevelIfNoSecondary(
            model.scene,
            model.renderOptions,
            selectedPatchIndex,
            secondarySelectedPatchIndex,
            model.selectedHierarchyLevel,
            cachedInteractionsForPrimaryPatch);
        GlutDebugPatchHierarchy::renderInteractionBetweenSelected(
            model.scene,
            selectedPatchIndex,
            secondarySelectedPatchIndex,
            cachedInteractionsForPrimaryPatch);
        glPopMatrix();
    }

    char hudModeText[256];
    std::snprintf(
        hudModeText,
        sizeof(hudModeText),
        "MODE: %s [m]",
        GlutDebugModeTools::modeName(model.mode));
    GlutHudConsole::printTextLine(hudModeText, 0, 0, model.width, model.height);

    char hudSelectedElementText[256];
    if ( selectedPatchIndex >= 0 ) {
        std::snprintf(
            hudSelectedElementText,
            sizeof(hudSelectedElementText),
            "Element %d/%d [1, 2, mouse click]",
            selectedPatchIndex + 1,
            totalElements);
    } else {
        std::snprintf(
            hudSelectedElementText,
            sizeof(hudSelectedElementText),
            "Element none/%d [1, 2, mouse click]",
            totalElements);
    }
    GlutHudConsole::printTextLine(hudSelectedElementText, 0, 1, model.width, model.height);

    if ( model.mode == GlutDebugMode::GALERKIN_ELEMENT_HIERARCHY ) {
        char hudSecondarySelectedElementText[256];
        if ( secondarySelectedPatchIndex >= 0 ) {
            std::snprintf(
                hudSecondarySelectedElementText,
                sizeof(hudSecondarySelectedElementText),
                "Secondary element %d/%d [5, 6, shift-click]",
                secondarySelectedPatchIndex + 1,
                totalElements);
        } else {
            std::snprintf(
                hudSecondarySelectedElementText,
                sizeof(hudSecondarySelectedElementText),
                "Secondary element none/%d [5, 6, shift-click]",
                totalElements);
        }
        GlutHudConsole::printTextLine(hudSecondarySelectedElementText, 0, 2, model.width, model.height);

        int maxHierarchyLevel = 0;
        if ( selectedPatchIndex >= 0 && model.scene != nullptr ) {
            maxHierarchyLevel = GlutDebugPatchHierarchy::maxLevelForSelectedPatch(
                model.scene,
                selectedPatchIndex);
        }

        int currentHierarchyLevel = model.selectedHierarchyLevel;
        if ( currentHierarchyLevel < 0 ) {
            currentHierarchyLevel = 0;
        }
        if ( currentHierarchyLevel > maxHierarchyLevel ) {
            currentHierarchyLevel = maxHierarchyLevel;
        }

        char currentLevelLabel[32];
        if ( selectedPatchIndex < 0 ) {
            std::snprintf(currentLevelLabel, sizeof(currentLevelLabel), "none");
        } else if ( currentHierarchyLevel == 0 ) {
            std::snprintf(currentLevelLabel, sizeof(currentLevelLabel), "base");
        } else {
            std::snprintf(currentLevelLabel, sizeof(currentLevelLabel), "%d", currentHierarchyLevel);
        }

        char hudSubdivisionText[256];
        std::snprintf(
            hudSubdivisionText,
            sizeof(hudSubdivisionText),
            "Patch subdivision level: %s/%d",
            currentLevelLabel,
            maxHierarchyLevel);
        GlutHudConsole::printTextLine(hudSubdivisionText, 0, 3, model.width, model.height);

        int primaryInteractionCount = 0;
        if ( selectedPatchIndex >= 0 && cachedInteractionsForPrimaryPatch != nullptr ) {
            primaryInteractionCount = cachedInteractionsForPrimaryPatch->size();
        }

        char hudPrimaryInteractionsText[256];
        if ( selectedPatchIndex >= 0 ) {
            std::snprintf(
                hudPrimaryInteractionsText,
                sizeof(hudPrimaryInteractionsText),
                "Primary element Iteration: %d",
                primaryInteractionCount);
        } else {
            std::snprintf(
                hudPrimaryInteractionsText,
                sizeof(hudPrimaryInteractionsText),
                "Primary element Iteration: none");
        }
        GlutHudConsole::printTextLine(hudPrimaryInteractionsText, 0, 4, model.width, model.height);
    }

    glutSwapBuffers();
}

void
GlutDebugTools::executeGlutGui(int argc, char *argv[]) {
    model.fullScreenApplied = false;

    activeGlutDebugToolsInstance() = this;

    glutInit(&argc, argv);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(model.width, model.height);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    int windowHandle = glutCreateWindow("RPK");
    if ( windowHandle == GL_FALSE ) {
        java::System::out.printf("ERROR: Can not open GLUT window, check OpenGL/GLUT setup.\n");
        java::System::exit(1);
    }

    model.renderOptions->frustumCulling = false;

    glutReshapeFunc(resizeCallbackBridge);
    glutKeyboardFunc(keypressCallbackBridge);
    glutSpecialFunc(extendedKeypressCallbackBridge);
    glutMouseFunc(mouseButtonCallbackBridge);
    glutMotionFunc(mouseMotionCallbackBridge);
    glutDisplayFunc(drawCallbackBridge);
    glutMainLoop();

    activeGlutDebugToolsInstance() = nullptr;
}

#endif
