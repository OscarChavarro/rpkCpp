#include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugToolsKeyControl.h"

#ifdef __APPLE__
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

#include "vsdk/toolkit/java/lang/System.h"
#include "vsdk/toolkit/java/util/ArrayList.txx"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugState.h"
#include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugMode.h"
#include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugPatchHierarchy.h"

bool
GlutDebugToolsKeyControl::isGalerkinPatchIndex(const Scene *scene, int patchIndex) {
    if ( scene == nullptr || scene->patchList == nullptr ) {
        return false;
    }
    if ( patchIndex < 0 || patchIndex >= scene->patchList->size() ) {
        return false;
    }

    const Patch *patch = scene->patchList->get(patchIndex);
    if ( patch == nullptr || patch->getRadianceData() == nullptr ) {
        return false;
    }

    return patch->getRadianceData()->className == ElementTypes::ELEMENT_GALERKIN;
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
            if ( GlutDebugToolsKeyControl::isGalerkinPatchIndex(scene, *selectedPatchIndex) ) {
                return;
            }

            int fallback = patchCount - 1;
            while ( fallback >= 0 && !GlutDebugToolsKeyControl::isGalerkinPatchIndex(scene, fallback) ) {
                fallback--;
            }
            *selectedPatchIndex = fallback;
            return;
        }
        if ( GlutDebugToolsKeyControl::isGalerkinPatchIndex(scene, nextPatchIndex) ) {
            *selectedPatchIndex = nextPatchIndex;
            return;
        }
    }
}

int
GlutDebugToolsKeyControl::selectedPatchMaxHierarchyLevel(const GlutDebugToolsModel &model) {
    if ( model.scene == nullptr || model.debugState == nullptr ) {
        return 0;
    }
    return GlutDebugPatchHierarchy::maxLevelForSelectedPatch(
        model.scene,
        model.debugState->primarySelectedPatch);
}

void
GlutDebugToolsKeyControl::clampHierarchyLevel(GlutDebugToolsModel &model) {
    if ( model.selectedHierarchyLevel < 0 ) {
        model.selectedHierarchyLevel = 0;
    }
}

bool
GlutDebugToolsKeyControl::handleKeypress(
    unsigned char keyChar,
    GlutDebugToolsModel &model,
    void (*printGalerkinElementForPatch)(const Scene *scene, int patchIndex))
{
    GlutDebugState *const debugState = model.debugState;

    switch ( keyChar ) {
        case 27:
            if ( model.memoryFreeCallBack != nullptr ) {
                model.memoryFreeCallBack(model.mgfContext);
            }
            java::System::exit(1);
            return false;
        case '0':
            if ( debugState == nullptr ) {
                return false;
            }
            debugState->showSelectedPathOnly = !debugState->showSelectedPathOnly;
            break;
        case '1':
            if ( debugState == nullptr ) {
                return false;
            }
            stepSelectedPatchIndex(&debugState->primarySelectedPatch, -1, model.scene);
            GlutDebugToolsKeyControl::clampHierarchyLevel(model);
            break;
        case '2':
            if ( debugState == nullptr ) {
                return false;
            }
            stepSelectedPatchIndex(&debugState->primarySelectedPatch, 1, model.scene);
            GlutDebugToolsKeyControl::clampHierarchyLevel(model);
            break;
        case '5':
            if ( debugState == nullptr ) {
                return false;
            }
            stepSelectedPatchIndex(&debugState->selectedSelectedPatch, -1, model.scene);
            break;
        case '6':
            if ( debugState == nullptr ) {
                return false;
            }
            stepSelectedPatchIndex(&debugState->selectedSelectedPatch, 1, model.scene);
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
            model.mode = GlutDebugModeTools::nextMode(model.mode);
            GlutDebugToolsKeyControl::clampHierarchyLevel(model);
            break;
        case 'f':
            model.fullScreen = !model.fullScreen;
            break;
        case ' ':
            if ( model.radianceMethod != nullptr && model.scene != nullptr && model.renderOptions != nullptr ) {
                model.radianceMethod->doStep(model.scene, model.renderOptions);
                model.selectedHierarchyLevel = GlutDebugPatchHierarchy::maxLevelAcrossScene(model.scene);
            }
            break;
        case 'e':
            if ( printGalerkinElementForPatch != nullptr && model.scene != nullptr && debugState != nullptr ) {
                printGalerkinElementForPatch(model.scene, debugState->primarySelectedPatch);
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

    GlutDebugState *const debugState = model.debugState;

    switch ( keyCode ) {
        case GLUT_KEY_F2:
            model.renderOptions->setDrawOutlines(!model.renderOptions->isDrawOutlines());
            break;
        case GLUT_KEY_F3:
            model.renderOptions->setDrawSurfaces(!model.renderOptions->isDrawSurfaces());
            break;
        case GLUT_KEY_F4:
            model.renderOptions->setDrawBoundingBoxes(!model.renderOptions->isDrawBoundingBoxes());
            break;
        case GLUT_KEY_F5:
            model.renderOptions->setDrawClusters(!model.renderOptions->isDrawClusters());
            break;
        case GLUT_KEY_LEFT:
            if ( debugState == nullptr ) {
                return false;
            }
            debugState->angleAroundViewportV += 1.0F;
            break;
        case GLUT_KEY_RIGHT:
            if ( debugState == nullptr ) {
                return false;
            }
            debugState->angleAroundViewportV -= 1.0F;
            break;
        case GLUT_KEY_DOWN:
            if ( debugState == nullptr ) {
                return false;
            }
            debugState->angleAroundViewportU += 1.0F;
            break;
        case GLUT_KEY_UP:
            if ( debugState == nullptr ) {
                return false;
            }
            debugState->angleAroundViewportU -= 1.0F;
            break;
        default:
            return false;
    }

    return true;
}
