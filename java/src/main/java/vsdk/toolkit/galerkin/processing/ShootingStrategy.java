package vsdk.toolkit.galerkin.processing;

import java.util.ArrayList;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.galerkin.GalerkinBasis;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinRadianceMethod;
import vsdk.toolkit.galerkin.GalerkinRole;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.render.Potential;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.skin.Element;
import vsdk.toolkit.skin.ElementFlags;
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

    private static Patch chooseRadianceShootingPatch(ArrayList<Patch> scenePatches, GalerkinState galerkinState) {
        Patch shootingPatch = null;
        Patch potentialShootingPatch = null;
        float maximumPower = 0.0f;
        float maximumPowerImportance = 0.0f;

        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            Patch patch = scenePatches.get(i);
            if ( patch == null || patch.radianceData == null || patch.radianceData.unShotRadiance == null
                 || patch.radianceData.unShotRadiance.length == 0 || patch.radianceData.unShotRadiance[0] == null ) {
                continue;
            }

            float power = (float)(Math.PI * patch.area * patch.radianceData.unShotRadiance[0].sumAbsComponents());
            if ( power > maximumPower ) {
                shootingPatch = patch;
                maximumPower = power;
            }

            if ( galerkinState.importanceDriven != 0 ) {
                // For importance-driven progressive refinement radiosity, choose the patch
                // with highest indirectly received potential times power.
                float powerImportance = (galerkinGetPotential(patch) - patch.directPotential) * power;
                if ( powerImportance > maximumPowerImportance ) {
                    potentialShootingPatch = patch;
                    maximumPowerImportance = powerImportance;
                }
            }
        }

        if ( galerkinState.importanceDriven != 0 && potentialShootingPatch != null ) {
            return potentialShootingPatch;
        }
        return shootingPatch;
    }

    private static void clearUnShotRadianceAndPotential(GalerkinElement elem) {
        if ( elem == null ) {
            return;
        }

        if ( elem.regularSubElements != null ) {
            for ( int i = 0; i < 4; i++ ) {
                Element child = elem.regularSubElements[i];
                if ( child instanceof GalerkinElement ) {
                    clearUnShotRadianceAndPotential((GalerkinElement)child);
                }
            }
        }

        for ( int i = 0; elem.irregularSubElements != null && i < elem.irregularSubElements.size(); i++ ) {
            Element child = elem.irregularSubElements.get(i);
            if ( child instanceof GalerkinElement ) {
                clearUnShotRadianceAndPotential((GalerkinElement)child);
            }
        }

        if ( elem.unShotRadiance != null ) {
            ColorRgb.arrayClear(elem.unShotRadiance, elem.basisSize);
        }
        elem.unShotPotential = 0.0f;
    }

    private static void patchPropagateUnShotRadianceAndPotential(
        Scene scene,
        Patch patch,
        GalerkinState galerkinState)
    {
        if ( scene == null || patch == null || galerkinState == null ) {
            return;
        }

        GalerkinElement topLevelElement = GalerkinElement.fromPatch(patch);
        if ( topLevelElement == null ) {
            return;
        }

        if ( (topLevelElement.flags & ElementFlags.INTERACTIONS_CREATED_MASK) == 0 ) {
            if ( galerkinState.clustered != 0 ) {
                LinkingClusteredStrategy.createInitialLinks(topLevelElement, GalerkinRole.SOURCE, galerkinState);
            }
            else {
                LinkingSimpleStrategy.createInitialLinks(
                    scene,
                    galerkinState,
                    GalerkinRole.SOURCE,
                    topLevelElement);
            }
            topLevelElement.flags |= ElementFlags.INTERACTIONS_CREATED_MASK;
        }

        // Recursively refines the interactions of the shooting patch and computes
        // radiance and potential transport.
        HierarchicalRefinementStrategy.refineInteractions(scene, topLevelElement, galerkinState);

        // Clear the un-shot radiance at all levels.
        clearUnShotRadianceAndPotential(topLevelElement);
    }

    private static float shootingPushPullPotential(GalerkinElement element, float down) {
        if ( element == null ) {
            return 0.0f;
        }

        if ( element.area != 0.0f ) {
            down += element.receivedPotential / element.area;
        }
        element.receivedPotential = 0.0f;

        float up = 0.0f;

        if ( element.regularSubElements == null
             && (element.irregularSubElements == null || element.irregularSubElements.isEmpty()) ) {
            up = down;
        }

        if ( element.regularSubElements != null ) {
            for ( int i = 0; i < 4; i++ ) {
                Element child = element.regularSubElements[i];
                if ( child instanceof GalerkinElement ) {
                    up += 0.25f * shootingPushPullPotential((GalerkinElement)child, down);
                }
            }
        }

        if ( element.irregularSubElements != null ) {
            for ( int j = 0; j < element.irregularSubElements.size(); j++ ) {
                Element child = element.irregularSubElements.get(j);
                if ( !(child instanceof GalerkinElement) ) {
                    continue;
                }
                GalerkinElement subElement = (GalerkinElement)child;
                if ( !element.isCluster() ) {
                    down = 0.0f;
                }
                // Don't push to irregular surface sub-elements.
                if ( element.area != 0.0f ) {
                    up += subElement.area / element.area * shootingPushPullPotential(subElement, down);
                }
            }
        }

        element.potential += up;
        element.unShotPotential += up;
        return up;
    }

    private static void patchUpdateRadianceAndPotential(Patch patch, GalerkinState galerkinState) {
        if ( patch == null || galerkinState == null ) {
            return;
        }

        GalerkinElement topLevelElement = GalerkinElement.fromPatch(patch);
        if ( topLevelElement == null ) {
            return;
        }

        if ( galerkinState.importanceDriven != 0 ) {
            shootingPushPullPotential(topLevelElement, 0.0f);
        }
        GalerkinBasis.pushPullRadiance(topLevelElement, galerkinState);

        if ( patch.radianceData != null && patch.radianceData.unShotRadiance != null
             && patch.radianceData.unShotRadiance.length > 0 && patch.radianceData.unShotRadiance[0] != null ) {
            galerkinState.ambientRadiance.addScaled(
                galerkinState.ambientRadiance,
                patch.area,
                patch.radianceData.unShotRadiance[0]);
        }
    }

    private static void doPropagate(Scene scene, Patch shootingPatch, GalerkinState galerkinState) {
        if ( scene == null || galerkinState == null || shootingPatch == null ) {
            return;
        }

        // Propagate the un-shot power of the shooting patch into the environment.
        patchPropagateUnShotRadianceAndPotential(scene, shootingPatch, galerkinState);

        // Recompute the colors of all patches, not only the patches that received
        // radiance from the shooting patch, since the ambient term has also changed.
        if ( galerkinState.clustered != 0 ) {
            if ( galerkinState.importanceDriven != 0 ) {
                shootingPushPullPotential(galerkinState.topCluster, 0.0f);
            }
            GalerkinBasis.pushPullRadiance(galerkinState.topCluster, galerkinState);
            if ( galerkinState.topCluster != null
                 && galerkinState.topCluster.unShotRadiance != null
                 && galerkinState.topCluster.unShotRadiance.length > 0
                 && galerkinState.topCluster.unShotRadiance[0] != null ) {
                ColorRgb ambient = galerkinState.topCluster.unShotRadiance[0];
                galerkinState.ambientRadiance.set(ambient.r, ambient.g, ambient.b);
            }
        }
        else {
            galerkinState.ambientRadiance.clear();
            for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
                patchUpdateRadianceAndPotential(scene.patchList.get(i), galerkinState);
            }
            galerkinState.ambientRadiance.scale(1.0f / Statistics.instance().radiance.totalArea);
        }

        for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
            GalerkinRadianceMethod.recomputePatchColor(scene.patchList.get(i));
        }
    }

    private static boolean propagateRadiance(Scene scene, GalerkinState galerkinState) {
        Patch shootingPatch = chooseRadianceShootingPatch(scene.patchList, galerkinState);
        if ( shootingPatch == null ) {
            return true;
        }

        doPropagate(scene, shootingPatch, galerkinState);
        return false;
    }

    private static void clusterUpdatePotential(GalerkinElement clusterElement) {
        if ( clusterElement == null || !clusterElement.isCluster() ) {
            return;
        }

        clusterElement.potential = 0.0f;
        clusterElement.unShotPotential = 0.0f;
        for ( int i = 0; clusterElement.irregularSubElements != null
                       && i < clusterElement.irregularSubElements.size(); i++ ) {
            Element child = clusterElement.irregularSubElements.get(i);
            if ( !(child instanceof GalerkinElement) ) {
                continue;
            }
            GalerkinElement subCluster = (GalerkinElement)child;
            clusterUpdatePotential(subCluster);
            clusterElement.potential += subCluster.area * subCluster.potential;
            clusterElement.unShotPotential += subCluster.area * subCluster.unShotPotential;
        }
        if ( clusterElement.area != 0.0f ) {
            clusterElement.potential /= clusterElement.area;
            clusterElement.unShotPotential /= clusterElement.area;
        }
    }

    private static Patch choosePotentialShootingPatch(ArrayList<Patch> scenePatches) {
        float maximumImportance = 0.0f;
        Patch shootingPatch = null;

        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            Patch patch = scenePatches.get(i);
            if ( patch == null ) {
                continue;
            }
            float importance = (float)(patch.area * Math.abs(galerkinGetUnShotPotential(patch)));
            if ( importance > maximumImportance ) {
                shootingPatch = patch;
                maximumImportance = importance;
            }
        }

        return shootingPatch;
    }

    private static void propagatePotential(Scene scene, GalerkinState galerkinState) {
        Patch shootingPatch = choosePotentialShootingPatch(scene.patchList);
        if ( shootingPatch != null ) {
            doPropagate(scene, shootingPatch, galerkinState);
        }
        else {
            System.err.printf("No patches with un-shot potential??\n");
        }
    }

    private static void shootingUpdateDirectPotential(GalerkinElement galerkinElement, float potentialIncrement) {
        if ( galerkinElement == null ) {
            return;
        }

        if ( galerkinElement.regularSubElements != null ) {
            for ( int i = 0; i < 4; i++ ) {
                Element child = galerkinElement.regularSubElements[i];
                if ( child instanceof GalerkinElement ) {
                    shootingUpdateDirectPotential((GalerkinElement)child, potentialIncrement);
                }
            }
        }
        galerkinElement.directPotential += potentialIncrement;
        galerkinElement.potential += potentialIncrement;
        galerkinElement.unShotPotential += potentialIncrement;
    }

    public static boolean doShootingStep(Scene scene, GalerkinState galerkinState, RenderOptions renderOptions) {
        if ( galerkinState.importanceDriven != 0 ) {
            if ( galerkinState.iterationNumber <= 1 || scene.camera.changed != 0 ) {
                Potential.updateDirectPotential(scene, renderOptions);
                for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
                    Patch patch = scene.patchList.get(i);
                    GalerkinElement topLevelElement = GalerkinElement.fromPatch(patch);
                    if ( topLevelElement == null ) {
                        continue;
                    }
                    float potentialIncrement = patch.directPotential - topLevelElement.directPotential;
                    shootingUpdateDirectPotential(topLevelElement, potentialIncrement);
                }
                scene.camera.changed = 0;
                if ( galerkinState.clustered != 0 ) {
                    clusterUpdatePotential(galerkinState.topCluster);
                }
            }
            propagatePotential(scene, galerkinState);
        }
        return propagateRadiance(scene, galerkinState);
    }
}
