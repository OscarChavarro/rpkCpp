package vsdk.toolkit.galerkin.processing;

import vsdk.toolkit.common.color.ColorRgb;

import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.galerkin.GalerkinBasis;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinIterationMethod;
import vsdk.toolkit.galerkin.GalerkinRadianceMethod;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.galerkin.GalerkinRole;
import vsdk.toolkit.render.Potential;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.ElementFlags;
import vsdk.toolkit.environment.geometry.elements.Patch;

public final class GatheringClusteredStrategy extends GatheringStrategy {
    private static double updatePotential(GalerkinElement cluster) {
        if ( (cluster.flags & ElementFlags.IS_CLUSTER_MASK) != 0 ) {
            cluster.potential = 0.0;
            for ( int i = 0; cluster.irregularSubElements != null && i < cluster.irregularSubElements.size(); i++ ) {
                if ( !(cluster.irregularSubElements.get(i) instanceof GalerkinElement) ) {
                    continue;
                }
                GalerkinElement subCluster = (GalerkinElement)cluster.irregularSubElements.get(i);
                cluster.potential += subCluster.area * GatheringClusteredStrategy.updatePotential(subCluster);
            }
            cluster.potential /= cluster.area;
        }
        return cluster.potential;
    }

    private static void updateClusterDirectPotential(GalerkinElement element, double potentialIncrement) {
        if ( element.regularSubElements != null ) {
            for ( int i = 0; i < 4; i++ ) {
                if ( element.regularSubElements[i] instanceof GalerkinElement ) {
                    GatheringClusteredStrategy.updateClusterDirectPotential( (GalerkinElement)element.regularSubElements[i], potentialIncrement);
                }
            }
        }
        element.directPotential += potentialIncrement;
        element.potential += potentialIncrement;
    }

    public GatheringClusteredStrategy() {
    }

    @Override
    public boolean doGatheringIteration(Scene scene, GalerkinState galerkinState, RendererConfiguration renderOptions) {
        if ( galerkinState.importanceDriven != 0
             && (galerkinState.iterationNumber <= 1 || scene.camera.changed != 0) ) {
            Potential.updateDirectPotential(scene, renderOptions);
            for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
                Patch patch = scene.patchList.get(i);
                GalerkinElement top = galerkinState.topLevelElement(patch);
                double potentialIncrement = scene.getDirectPotentials().get(patch.getFaceIndex()) - top.directPotential;
                GatheringClusteredStrategy.updateClusterDirectPotential(top, potentialIncrement);
            }
            GatheringClusteredStrategy.updatePotential(galerkinState.topCluster);
            scene.camera.changed = 0;
        }

        System.out.printf("Galerkin (clustered) iteration %d\n", galerkinState.iterationNumber);

        // Initial linking stage is replaced by the creation of a self-link between
        // the whole scene and itself
        if ( galerkinState.iterationNumber <= 1 ) {
            LinkingClusteredStrategy.createInitialLinks(galerkinState.topCluster, GalerkinRole.RECEIVER, galerkinState);
        }

        double userErrorThreshold = galerkinState.relLinkErrorThreshold;

        // Refines and computes light transport over the refined links
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.PARALLEL_JACOBI ) {
            HierarchicalRefinementStrategy.refineInteractionsInParallel(scene, galerkinState.topCluster, galerkinState);
        }
        else {
            HierarchicalRefinementStrategy.refineInteractions(scene, galerkinState.topCluster, galerkinState);
        }

        galerkinState.relLinkErrorThreshold = userErrorThreshold;

        // Push received radiance down and pull up
        GalerkinBasis.pushPullRadiance(galerkinState.topCluster, galerkinState);

        if ( galerkinState.importanceDriven != 0 ) {
            GatheringStrategy.pushPullPotential(galerkinState.topCluster, 0.0, scene.getDirectPotentials());
        }

        // No visualisation with ambient term for gathering radiosity algorithms
        galerkinState.ambientRadiance = new ColorRgb(0.0, 0.0, 0.0);

        // Update the display colors of the patches
        for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
            GalerkinRadianceMethod.recomputePatchColor(scene.patchList.get(i), scene.getFaceColors());
        }
        return false; // Never done
    }
}
