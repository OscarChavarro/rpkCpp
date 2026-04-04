package vsdk.toolkit.galerkin.processing;

import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.skin.Patch;

/**
Reference:
[COHE1993] Cohen, M. Wallace, J. "Radiosity and Realistic Image Synthesis",
     Academic Press Professional, 1993.
*/

/**
See [COHE1993].5.3.3. section
*/
public class ShootingStrategy {
    private static float galerkinGetPotential(Patch patch) {
        return (patch != null && patch.radianceData instanceof GalerkinElement)
            ? ((GalerkinElement)patch.radianceData).potential : 0.0f;
    }

    private static float galerkinGetUnShotPotential(Patch patch) {
        return (patch != null && patch.radianceData instanceof GalerkinElement)
            ? ((GalerkinElement)patch.radianceData).unShotPotential : 0.0f;
    }

    private static Patch chooseRadianceShootingPatch(java.util.ArrayList<Patch> scenePatches, GalerkinState galerkinState) {
        return null;
    }

    private static void clearUnShotRadianceAndPotential(GalerkinElement elem) {
    }

    private static void patchPropagateUnShotRadianceAndPotential(
        Scene scene,
        Patch patch,
        GalerkinState galerkinState)
    {
    }

    private static float shootingPushPullPotential(GalerkinElement element, float down) {
        return 0.0f;
    }

    private static void patchUpdateRadianceAndPotential(Patch patch, GalerkinState galerkinState) {
    }

    private static void doPropagate(Scene scene, Patch shootingPatch, GalerkinState galerkinState) {
    }

    private static boolean propagateRadiance(Scene scene, GalerkinState galerkinState) {
        return true;
    }

    private static void clusterUpdatePotential(GalerkinElement clusterElement) {
    }

    private static Patch choosePotentialShootingPatch(java.util.ArrayList<Patch> scenePatches) {
        return null;
    }

    private static void propagatePotential(Scene scene, GalerkinState galerkinState) {
    }

    private static void shootingUpdateDirectPotential(GalerkinElement galerkinElement, float potentialIncrement) {
    }

    public static boolean doShootingStep(Scene scene, GalerkinState galerkinState, RenderOptions renderOptions) {
        return true;
    }
}
