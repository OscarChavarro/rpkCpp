package vsdk.toolkit.galerkin.processing;

import vsdk.toolkit.scene.DirectPotentialTable;
import vsdk.toolkit.galerkin.ColorRgbMath;
import java.util.ArrayList;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.galerkin.GalerkinBasis;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinRadianceMethod;
import vsdk.toolkit.galerkin.GalerkinRole;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.render.Potential;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.Element;
import vsdk.toolkit.environment.geometry.elements.ElementFlags;
import vsdk.toolkit.environment.geometry.elements.Patch;

/**
Reference:
[COHE1993] Cohen, M. Wallace, J. "Radiosity and Realistic Image Synthesis",
     Academic Press Professional, 1993.
*/

/**
See [COHE1993].5.3.3. section
*/
public class ShootingStrategy {
    private static double galerkinGetPotential(GalerkinState galerkinState, Patch patch) {
        return galerkinState.topLevelElement(patch).potential;
    }

    private static double galerkinGetUnShotPotential(GalerkinState galerkinState, Patch patch) {
        return galerkinState.topLevelElement(patch).unShotPotential;
    }

    private static Patch chooseRadianceShootingPatch(ArrayList<Patch> scenePatches, DirectPotentialTable directPotentials, GalerkinState galerkinState) {
        Patch shootingPatch = null;
        Patch potentialShootingPatch = null;
        double maximumPower = 0.0;
        double maximumPowerImportance = 0.0;

        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            Patch patch = scenePatches.get(i);
            double power = (double)(Math.PI * patch.getArea() * galerkinState.topLevelElement(patch).unShotRadiance[0].sumAbsComponents());
            if ( power > maximumPower ) {
                shootingPatch = patch;
                maximumPower = power;
            }

            if ( galerkinState.importanceDriven != 0 ) {
                // For importance-driven progressive refinement radiosity, choose the patch
                // with highest indirectly received potential times power.
                double powerImportance = (galerkinGetPotential(galerkinState, patch) - directPotentials.get(patch.getFaceIndex())) * power;
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
            ColorRgbMath.arrayClear(elem.unShotRadiance, elem.basisSize);
        }
        elem.unShotPotential = 0.0;
    }

    private static void patchPropagateUnShotRadianceAndPotential(
        Scene scene,
        Patch patch,
        GalerkinState galerkinState)
    {
        if ( scene == null || patch == null || galerkinState == null ) {
            return;
        }

        GalerkinElement topLevelElement = GalerkinElement.fromPatch(galerkinState, patch);
        if ( topLevelElement == null ) {
            return;
        }

        if ( (topLevelElement.flags & ElementFlags.INTERACTIONS_CREATED_MASK) == 0 ) {
            if ( galerkinState.clustered != 0 ) {
                LinkingClusteredStrategy.createInitialLinks(topLevelElement, GalerkinRole.SOURCE, galerkinState);
            }
            else {
                LinkingSimpleStrategy.createInitialLinks( scene, galerkinState, GalerkinRole.SOURCE, topLevelElement);
            }
            topLevelElement.flags |= ElementFlags.INTERACTIONS_CREATED_MASK;
        }

        // Recursively refines the interactions of the shooting patch and computes
        // radiance and potential transport.
        HierarchicalRefinementStrategy.refineInteractions(scene, topLevelElement, galerkinState);

        // Clear the un-shot radiance at all levels.
        clearUnShotRadianceAndPotential(topLevelElement);
    }

    private static double shootingPushPullPotential(GalerkinElement element, double down) {
        if ( element == null ) {
            return 0.0;
        }

        if ( element.area != 0.0 ) {
            down += element.receivedPotential / element.area;
        }
        element.receivedPotential = 0.0;

        double up = 0.0;

        if ( element.regularSubElements == null
             && (element.irregularSubElements == null || element.irregularSubElements.isEmpty()) ) {
            up = down;
        }

        if ( element.regularSubElements != null ) {
            for ( int i = 0; i < 4; i++ ) {
                Element child = element.regularSubElements[i];
                if ( child instanceof GalerkinElement ) {
                    up += 0.25 * shootingPushPullPotential((GalerkinElement)child, down);
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
                    down = 0.0;
                }
                // Don't push to irregular surface sub-elements.
                if ( element.area != 0.0 ) {
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

        GalerkinElement topLevelElement = GalerkinElement.fromPatch(galerkinState, patch);
        if ( topLevelElement == null ) {
            return;
        }

        if ( galerkinState.importanceDriven != 0 ) {
            shootingPushPullPotential(topLevelElement, 0.0);
        }
        GalerkinBasis.pushPullRadiance(topLevelElement, galerkinState);

        galerkinState.ambientRadiance = galerkinState.ambientRadiance.addScaled(patch.getArea(), topLevelElement.unShotRadiance[0]);
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
                shootingPushPullPotential(galerkinState.topCluster, 0.0);
            }
            GalerkinBasis.pushPullRadiance(galerkinState.topCluster, galerkinState);
            if ( galerkinState.topCluster != null
                 && galerkinState.topCluster.unShotRadiance != null
                 && galerkinState.topCluster.unShotRadiance.length > 0
                 && galerkinState.topCluster.unShotRadiance[0] != null ) {
                ColorRgb ambient = galerkinState.topCluster.unShotRadiance[0];
                galerkinState.ambientRadiance = new ColorRgb(ambient.getR(), ambient.getG(), ambient.getB());
            }
        }
        else {
            galerkinState.ambientRadiance = new ColorRgb(0.0, 0.0, 0.0);
            for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
                patchUpdateRadianceAndPotential(scene.patchList.get(i), galerkinState);
            }
            galerkinState.ambientRadiance = galerkinState.ambientRadiance.multiply(1.0 / Statistics.instance().radiance.totalArea);
        }

        for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
            GalerkinRadianceMethod.recomputePatchColor(scene.patchList.get(i), scene.getFaceColors());
        }
    }

    private static boolean propagateRadiance(Scene scene, GalerkinState galerkinState) {
        Patch shootingPatch = chooseRadianceShootingPatch(scene.patchList, scene.getDirectPotentials(), galerkinState);
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

        clusterElement.potential = 0.0;
        clusterElement.unShotPotential = 0.0;
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
        if ( clusterElement.area != 0.0 ) {
            clusterElement.potential /= clusterElement.area;
            clusterElement.unShotPotential /= clusterElement.area;
        }
    }

    private static Patch choosePotentialShootingPatch(ArrayList<Patch> scenePatches, GalerkinState galerkinState) {
        double maximumImportance = 0.0;
        Patch shootingPatch = null;

        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            Patch patch = scenePatches.get(i);
            if ( patch == null ) {
                continue;
            }
            double importance = (double)(patch.getArea() * Math.abs(galerkinGetUnShotPotential(galerkinState, patch)));
            if ( importance > maximumImportance ) {
                shootingPatch = patch;
                maximumImportance = importance;
            }
        }

        return shootingPatch;
    }

    private static void propagatePotential(Scene scene, GalerkinState galerkinState) {
        Patch shootingPatch = choosePotentialShootingPatch(scene.patchList, galerkinState);
        if ( shootingPatch != null ) {
            doPropagate(scene, shootingPatch, galerkinState);
        }
        else {
            System.err.printf("No patches with un-shot potential??\n");
        }
    }

    private static void shootingUpdateDirectPotential(GalerkinElement galerkinElement, double potentialIncrement) {
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

    public static boolean doShootingStep(Scene scene, GalerkinState galerkinState, RendererConfiguration renderOptions) {
        if ( galerkinState.importanceDriven != 0 ) {
            if ( galerkinState.iterationNumber <= 1 || scene.camera.changed != 0 ) {
                Potential.updateDirectPotential(scene, renderOptions);
                for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
                    Patch patch = scene.patchList.get(i);
                    GalerkinElement topLevelElement = GalerkinElement.fromPatch(galerkinState, patch);
                    if ( topLevelElement == null ) {
                        continue;
                    }
                    double potentialIncrement = scene.getDirectPotentials().get(patch.getFaceIndex()) - topLevelElement.directPotential;
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
