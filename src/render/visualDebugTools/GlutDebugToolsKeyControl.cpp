#include "render/visualDebugTools/GlutDebugToolsKeyControl.h"

#ifdef __APPLE__
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "scene/Scene.h"
#include "scene/RadianceMethod.h"
#include "render/visualDebugTools/GlutDebugState.h"
#include "render/visualDebugTools/GlutDebugMode.h"
#include "render/visualDebugTools/GlutDebugPatchHierarchy.h"

void
GlutDebugToolsKeyControl::printSelectedPatchState() {
    if ( GLOBAL_render_glutDebugState.showSelectedPathOnly ) {
        java::lang::System::out.printf("Selected patch: %d\n", GLOBAL_render_glutDebugState.selectedPatch);
    } else {
        java::lang::System::out.printf("Selected patch: ALL\n");
    }
}

int
GlutDebugToolsKeyControl::selectedPatchMaxHierarchyLevel(const GlutDebugToolsModel &model) {
    if ( model.scene == nullptr ) {
        return 0;
    }
    return GlutDebugPatchHierarchy::maxLevelForSelectedPatch(
        model.scene,
        GLOBAL_render_glutDebugState.selectedPatch);
}

void
GlutDebugToolsKeyControl::clampHierarchyLevel(GlutDebugToolsModel &model) {
    const int maxHierarchyLevel = GlutDebugToolsKeyControl::selectedPatchMaxHierarchyLevel(model);
    if ( model.selectedHierarchyLevel < 0 ) {
        model.selectedHierarchyLevel = 0;
    }
    if ( model.selectedHierarchyLevel > maxHierarchyLevel ) {
        model.selectedHierarchyLevel = maxHierarchyLevel;
    }
}

bool
GlutDebugToolsKeyControl::handleKeypress(
    unsigned char keyChar,
    GlutDebugToolsModel &model,
    void (*printGalerkinElementForPatch)(const Scene *scene, int patchIndex))
{
    switch ( keyChar ) {
        case 27:
            if ( model.memoryFreeCallBack != nullptr ) {
                model.memoryFreeCallBack(model.mgfContext);
            }
            java::lang::System::exit(1);
            return false;
        case '0':
            GLOBAL_render_glutDebugState.showSelectedPathOnly = !GLOBAL_render_glutDebugState.showSelectedPathOnly;
            break;
        case '1':
            GLOBAL_render_glutDebugState.selectedPatch--;
            if ( GLOBAL_render_glutDebugState.selectedPatch < 0 ) {
                GLOBAL_render_glutDebugState.selectedPatch = 0;
            }
            GlutDebugToolsKeyControl::clampHierarchyLevel(model);
            break;
        case '2':
            GLOBAL_render_glutDebugState.selectedPatch++;
            if ( model.scene != nullptr
                 && model.scene->patchList != nullptr
                 && GLOBAL_render_glutDebugState.selectedPatch >= model.scene->patchList->size() ) {
                GLOBAL_render_glutDebugState.selectedPatch = static_cast<int>(model.scene->patchList->size() - 1);
            }
            GlutDebugToolsKeyControl::clampHierarchyLevel(model);
            break;
        case '3':
            if ( model.mode != GlutDebugMode::GALERKIN_ELEMENT_HIERARCHY ) {
                return false;
            }
            if ( model.selectedHierarchyLevel > 0 ) {
                model.selectedHierarchyLevel--;
            }
            break;
        case '4':
            if ( model.mode != GlutDebugMode::GALERKIN_ELEMENT_HIERARCHY ) {
                return false;
            }
            GlutDebugToolsKeyControl::clampHierarchyLevel(model);
            if ( model.selectedHierarchyLevel < GlutDebugToolsKeyControl::selectedPatchMaxHierarchyLevel(model) ) {
                model.selectedHierarchyLevel++;
            }
            break;
        case 'm':
            model.mode = nextGlutDebugMode(model.mode);
            GlutDebugToolsKeyControl::clampHierarchyLevel(model);
            java::lang::System::out.printf("MODE: %s\n", glutDebugModeName(model.mode));
            break;
        case 'f':
            model.fullScreen = !model.fullScreen;
            java::lang::System::out.printf("Fullscreen: %s\n", model.fullScreen ? "ON" : "OFF");
            break;
        case ' ':
            if ( model.radianceMethod != nullptr && model.scene != nullptr && model.renderOptions != nullptr ) {
                model.radianceMethod->doStep(model.scene, model.renderOptions);
            }
            break;
        case 'e':
            if ( printGalerkinElementForPatch != nullptr && model.scene != nullptr ) {
                printGalerkinElementForPatch(model.scene, GLOBAL_render_glutDebugState.selectedPatch);
            }
            break;
        case 'p':
            if ( model.scene != nullptr ) {
                model.scene->print();
            }
            break;
        default:
            return false;
    }

    GlutDebugToolsKeyControl::printSelectedPatchState();
    if ( model.mode == GlutDebugMode::GALERKIN_ELEMENT_HIERARCHY ) {
        const int maxHierarchyLevel = GlutDebugToolsKeyControl::selectedPatchMaxHierarchyLevel(model);
        java::lang::System::out.printf(
            "Hierarchy level: %d/%d\n",
            model.selectedHierarchyLevel,
            maxHierarchyLevel);
    }
    return true;
}

bool
GlutDebugToolsKeyControl::handleExtendedKeypress(int keyCode, GlutDebugToolsModel &model) {
    if ( model.renderOptions == nullptr ) {
        return false;
    }

    switch ( keyCode ) {
        case GLUT_KEY_F2:
            model.renderOptions->drawOutlines = !model.renderOptions->drawOutlines;
            break;
        case GLUT_KEY_F3:
            model.renderOptions->drawSurfaces = !model.renderOptions->drawSurfaces;
            break;
        case GLUT_KEY_F4:
            model.renderOptions->drawBoundingBoxes = !model.renderOptions->drawBoundingBoxes;
            break;
        case GLUT_KEY_F5:
            model.renderOptions->drawClusters = !model.renderOptions->drawClusters;
            break;
        case GLUT_KEY_LEFT:
            GLOBAL_render_glutDebugState.angleAroundViewportV += 1.0f;
            break;
        case GLUT_KEY_RIGHT:
            GLOBAL_render_glutDebugState.angleAroundViewportV -= 1.0f;
            break;
        case GLUT_KEY_DOWN:
            GLOBAL_render_glutDebugState.angleAroundViewportU += 1.0f;
            break;
        case GLUT_KEY_UP:
            GLOBAL_render_glutDebugState.angleAroundViewportU -= 1.0f;
            break;
        default:
            return false;
    }

    return true;
}
