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
#include "skin/Patch.h"
#include "render/visualDebugTools/GlutDebugState.h"
#include "render/visualDebugTools/GlutDebugMode.h"
#include "render/visualDebugTools/GlutDebugPatchHierarchy.h"

namespace {

bool
isGalerkinPatchIndex(const Scene *scene, int patchIndex) {
    if ( scene == nullptr || scene->patchList == nullptr ) {
        return false;
    }
    if ( patchIndex < 0 || patchIndex >= scene->patchList->size() ) {
        return false;
    }

    const Patch *patch = scene->patchList->get(patchIndex);
    if ( patch == nullptr || patch->radianceData == nullptr ) {
        return false;
    }

    return patch->radianceData->className == ElementTypes::ELEMENT_GALERKIN;
}

}

void
GlutDebugToolsKeyControl::stepSelectedPatchIndex(
    int *selectedPatchIndex,
    int delta,
    const Scene *scene)
{
    if ( selectedPatchIndex == nullptr ) {
        return;
    }
    if ( delta == 0 ) {
        return;
    }

    if ( scene == nullptr || scene->patchList == nullptr || scene->patchList->size() <= 0 ) {
        *selectedPatchIndex = -1;
        return;
    }

    const int patchCount = scene->patchList->size();
    int nextPatchIndex = *selectedPatchIndex;
    if ( nextPatchIndex < -1 ) {
        nextPatchIndex = -1;
    }

    const int step = delta < 0 ? -1 : 1;
    while ( true ) {
        nextPatchIndex += step;
        if ( nextPatchIndex < 0 ) {
            *selectedPatchIndex = -1;
            return;
        }
        if ( nextPatchIndex >= patchCount ) {
            if ( isGalerkinPatchIndex(scene, *selectedPatchIndex) ) {
                return;
            }

            int fallback = patchCount - 1;
            while ( fallback >= 0 && !isGalerkinPatchIndex(scene, fallback) ) {
                fallback--;
            }
            *selectedPatchIndex = fallback;
            return;
        }
        if ( isGalerkinPatchIndex(scene, nextPatchIndex) ) {
            *selectedPatchIndex = nextPatchIndex;
            return;
        }
    }
}

int
GlutDebugToolsKeyControl::selectedPatchMaxHierarchyLevel(const GlutDebugToolsModel &model) {
    if ( model.scene == nullptr ) {
        return 0;
    }
    return GlutDebugPatchHierarchy::maxLevelForSelectedPatch(
        model.scene,
        GLOBAL_render_glutDebugState.primarySelectedPatch);
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
            stepSelectedPatchIndex(&GLOBAL_render_glutDebugState.primarySelectedPatch, -1, model.scene);
            GlutDebugToolsKeyControl::clampHierarchyLevel(model);
            break;
        case '2':
            stepSelectedPatchIndex(&GLOBAL_render_glutDebugState.primarySelectedPatch, 1, model.scene);
            GlutDebugToolsKeyControl::clampHierarchyLevel(model);
            break;
        case '5':
            stepSelectedPatchIndex(&GLOBAL_render_glutDebugState.selectedSelectedPatch, -1, model.scene);
            break;
        case '6':
            stepSelectedPatchIndex(&GLOBAL_render_glutDebugState.selectedSelectedPatch, 1, model.scene);
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
            break;
        case 'f':
            model.fullScreen = !model.fullScreen;
            break;
        case ' ':
            if ( model.radianceMethod != nullptr && model.scene != nullptr && model.renderOptions != nullptr ) {
                model.radianceMethod->doStep(model.scene, model.renderOptions);
            }
            break;
        case 'e':
            if ( printGalerkinElementForPatch != nullptr && model.scene != nullptr ) {
                printGalerkinElementForPatch(model.scene, GLOBAL_render_glutDebugState.primarySelectedPatch);
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
