package vsdk.toolkit.render.jogl.visualDebugTools;

import java.awt.event.KeyEvent;
import java.util.function.BiConsumer;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.ElementTypes;
import vsdk.toolkit.environment.geometry.elements.Patch;

public final class GlutDebugToolsKeyControl {
    private GlutDebugToolsKeyControl() {
    }

    private static boolean isGalerkinPatchIndex(Scene scene, int patchIndex) {
        if ( scene == null || scene.patchList == null ) {
            return false;
        }
        if ( patchIndex < 0 || patchIndex >= scene.patchList.size() ) {
            return false;
        }

        Patch patch = scene.patchList.get(patchIndex);
        if ( patch == null || patch.radianceData == null ) {
            return false;
        }
        return patch.radianceData.className == ElementTypes.ELEMENT_GALERKIN;
    }

    private static void stepSelectedPatchIndex(int[] selectedPatchIndex, int delta, Scene scene) {
        if ( selectedPatchIndex == null || selectedPatchIndex.length == 0 || delta == 0 ) {
            return;
        }

        if ( scene == null || scene.patchList == null || scene.patchList.isEmpty() ) {
            selectedPatchIndex[0] = -1;
            return;
        }

        int patchCount = scene.patchList.size();
        int nextPatchIndex = selectedPatchIndex[0];
        if ( nextPatchIndex < -1 ) {
            nextPatchIndex = -1;
        }

        int step = delta < 0 ? -1 : 1;
        while ( true ) {
            nextPatchIndex += step;
            if ( nextPatchIndex < 0 ) {
                selectedPatchIndex[0] = -1;
                return;
            }
            if ( nextPatchIndex >= patchCount ) {
                if ( isGalerkinPatchIndex(scene, selectedPatchIndex[0]) ) {
                    return;
                }
                int fallback = patchCount - 1;
                while ( fallback >= 0 && !isGalerkinPatchIndex(scene, fallback) ) {
                    fallback--;
                }
                selectedPatchIndex[0] = fallback;
                return;
            }
            if ( isGalerkinPatchIndex(scene, nextPatchIndex) ) {
                selectedPatchIndex[0] = nextPatchIndex;
                return;
            }
        }
    }

    private static int selectedPatchMaxHierarchyLevel(GlutDebugToolsModel model) {
        if ( model.scene == null || model.debugState == null ) {
            return 0;
        }
        return GlutDebugPatchHierarchy.maxLevelForSelectedPatch(model.scene, model.debugState.primarySelectedPatch);
    }

    private static void clampHierarchyLevel(GlutDebugToolsModel model) {
        int maxHierarchyLevel = selectedPatchMaxHierarchyLevel(model);
        if ( model.selectedHierarchyLevel < 0 ) {
            model.selectedHierarchyLevel = 0;
        }
        if ( model.selectedHierarchyLevel > maxHierarchyLevel ) {
            model.selectedHierarchyLevel = maxHierarchyLevel;
        }
    }

    public static boolean handleKeypress(char keyChar, GlutDebugToolsModel model, BiConsumer<Scene, Integer> printGalerkinElementForPatch) {
        GlutDebugState debugState = model.debugState;

        switch ( keyChar ) {
            case 27:
                if ( model.memoryFreeCallBack != null ) {
                    model.memoryFreeCallBack.accept(model.mgfContext);
                }
                System.exit(1);
                return false;
            case '0':
                if ( debugState == null ) {
                    return false;
                }
                debugState.showSelectedPathOnly = !debugState.showSelectedPathOnly;
                break;
            case '1':
                if ( debugState == null ) {
                    return false;
                }
                int[] p1 = new int[] {debugState.primarySelectedPatch};
                stepSelectedPatchIndex(p1, -1, model.scene);
                debugState.primarySelectedPatch = p1[0];
                clampHierarchyLevel(model);
                break;
            case '2':
                if ( debugState == null ) {
                    return false;
                }
                int[] p2 = new int[] {debugState.primarySelectedPatch};
                stepSelectedPatchIndex(p2, 1, model.scene);
                debugState.primarySelectedPatch = p2[0];
                clampHierarchyLevel(model);
                break;
            case '5':
                if ( debugState == null ) {
                    return false;
                }
                int[] p5 = new int[] {debugState.selectedSelectedPatch};
                stepSelectedPatchIndex(p5, -1, model.scene);
                debugState.selectedSelectedPatch = p5[0];
                break;
            case '6':
                if ( debugState == null ) {
                    return false;
                }
                int[] p6 = new int[] {debugState.selectedSelectedPatch};
                stepSelectedPatchIndex(p6, 1, model.scene);
                debugState.selectedSelectedPatch = p6[0];
                break;
            case '3':
                if ( model.mode != GlutDebugMode.GALERKIN_ELEMENT_HIERARCHY ) {
                    return false;
                }
                if ( model.selectedHierarchyLevel > 0 ) {
                    model.selectedHierarchyLevel--;
                }
                break;
            case '4':
                if ( model.mode != GlutDebugMode.GALERKIN_ELEMENT_HIERARCHY ) {
                    return false;
                }
                clampHierarchyLevel(model);
                if ( model.selectedHierarchyLevel < selectedPatchMaxHierarchyLevel(model) ) {
                    model.selectedHierarchyLevel++;
                }
                break;
            case 'm':
                model.mode = GlutDebugModeTools.nextMode(model.mode);
                clampHierarchyLevel(model);
                break;
            case 'f':
                model.fullScreen = !model.fullScreen;
                break;
            case ' ':
                if ( model.radianceMethod != null && model.scene != null && model.renderOptions != null ) {
                    model.radianceMethod.doStep(model.scene, model.renderOptions);
                }
                break;
            case 'e':
                if ( printGalerkinElementForPatch != null && model.scene != null && debugState != null ) {
                    printGalerkinElementForPatch.accept(model.scene, debugState.primarySelectedPatch);
                }
                break;
            case 'p':
                if ( model.scene != null ) {
                    model.scene.print();
                }
                break;
            default:
                return false;
        }

        return true;
    }

    public static boolean handleExtendedKeypress(int keyCode, GlutDebugToolsModel model) {
        if ( model.renderOptions == null ) {
            return false;
        }

        GlutDebugState debugState = model.debugState;

        switch ( keyCode ) {
            case KeyEvent.VK_F2:
                model.renderOptions.drawOutlines = !model.renderOptions.drawOutlines;
                break;
            case KeyEvent.VK_F3:
                model.renderOptions.drawSurfaces = !model.renderOptions.drawSurfaces;
                break;
            case KeyEvent.VK_F4:
                model.renderOptions.drawBoundingBoxes = !model.renderOptions.drawBoundingBoxes;
                break;
            case KeyEvent.VK_F5:
                model.renderOptions.drawClusters = !model.renderOptions.drawClusters;
                break;
            case KeyEvent.VK_LEFT:
                if ( debugState == null ) {
                    return false;
                }
                debugState.angleAroundViewportV += 1.0f;
                break;
            case KeyEvent.VK_RIGHT:
                if ( debugState == null ) {
                    return false;
                }
                debugState.angleAroundViewportV -= 1.0f;
                break;
            case KeyEvent.VK_DOWN:
                if ( debugState == null ) {
                    return false;
                }
                debugState.angleAroundViewportU += 1.0f;
                break;
            case KeyEvent.VK_UP:
                if ( debugState == null ) {
                    return false;
                }
                debugState.angleAroundViewportU -= 1.0f;
                break;
            default:
                return false;
        }

        return true;
    }
}
