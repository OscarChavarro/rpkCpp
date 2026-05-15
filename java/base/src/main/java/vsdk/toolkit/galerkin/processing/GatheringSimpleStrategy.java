package vsdk.toolkit.galerkin.processing;

import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinIterationMethod;
import vsdk.toolkit.galerkin.GalerkinRadianceMethod;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.galerkin.GalerkinRole;
import vsdk.toolkit.render.Potential;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.ElementFlags;
import vsdk.toolkit.environment.geometry.elements.Patch;

public final class GatheringSimpleStrategy extends GatheringStrategy {
    private static void patchUpdatePotential(Patch patch) {
        GalerkinElement topLevelElement = GalerkinElement.fromPatch(patch);
        GatheringStrategy.pushPullPotential(topLevelElement, 0.0f);
    }

    private static void patchUpdateRadiance(Patch patch, GalerkinState galerkinState) {
        GalerkinElement topLevelElement = GalerkinElement.fromPatch(patch);
        vsdk.toolkit.galerkin.GalerkinBasis.pushPullRadiance(topLevelElement, galerkinState);
        GalerkinRadianceMethod.recomputePatchColor(patch);
    }

    private static void patchLazyCreateInteractions(
        Scene scene,
        Patch patch,
        GalerkinState galerkinState)
    {
        GalerkinElement topLevelElement = GalerkinElement.fromPatch(patch);

        if ( !topLevelElement.radiance[0].isBlack()
             && (topLevelElement.flags & ElementFlags.INTERACTIONS_CREATED_MASK) == 0 ) {
            LinkingSimpleStrategy.createInitialLinks(
                scene,
                galerkinState,
                GalerkinRole.SOURCE,
                topLevelElement);
            topLevelElement.flags |= ElementFlags.INTERACTIONS_CREATED_MASK;
        }
    }

    private static void patchGather(
        Patch patch,
        Scene scene,
        GalerkinState galerkinState)
    {
        GalerkinElement topLevelElement = GalerkinElement.fromPatch(patch);

        // Don't gather to patches without importance. This optimisation can not
        // be combined with lazy linking based on radiance
        if ( galerkinState.importanceDriven != 0
             && topLevelElement.potential < Statistics.instance().potential.maxDirectPotential * Numeric.EPSILON ) {
            return;
        }

        if ( (galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL
              || galerkinState.lazyLinking == 0
              || galerkinState.importanceDriven != 0)
             && (topLevelElement.flags & ElementFlags.INTERACTIONS_CREATED_MASK) == 0 ) {
            LinkingSimpleStrategy.createInitialLinks(
                scene,
                galerkinState,
                GalerkinRole.RECEIVER,
                topLevelElement);
            topLevelElement.flags |= ElementFlags.INTERACTIONS_CREATED_MASK;
        }

        // Refine the interactions and compute light transport at the leaves
        HierarchicalRefinementStrategy.refineInteractions(scene, topLevelElement, galerkinState);

        // Immediately convert received radiance into radiance, make the representation
        // consistent and recompute the color of the patch when doing Gauss-Seidel.
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL ) {
            GatheringSimpleStrategy.patchUpdateRadiance(patch, galerkinState);
        }
    }

    public GatheringSimpleStrategy() {
    }

    @Override
    public boolean doGatheringIteration(Scene scene, GalerkinState galerkinState, RenderOptions renderOptions) {
        if ( galerkinState.importanceDriven != 0
             && (galerkinState.iterationNumber <= 1 || scene.camera.changed != 0) ) {
            Potential.updateDirectPotential(scene, renderOptions);
            scene.camera.changed = 0;
        }

        // Not importance-driven Jacobi iterations with lazy linking
        if ( galerkinState.galerkinIterationMethod != GalerkinIterationMethod.GAUSS_SEIDEL
             && galerkinState.lazyLinking != 0
             && galerkinState.importanceDriven == 0 ) {
            for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
                GatheringSimpleStrategy.patchLazyCreateInteractions(
                    scene,
                    scene.patchList.get(i),
                    galerkinState);
            }
        }

        // No visualisation with ambient term for gathering radiosity algorithms
        galerkinState.ambientRadiance.clear();

        // One iteration = gather to all patches
        for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
            GatheringSimpleStrategy.patchGather(scene.patchList.get(i), scene, galerkinState);
        }

        // Update the radiosity after gathering to all patches with Jacobi
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.JACOBI ) {
            for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
                GatheringSimpleStrategy.patchUpdateRadiance(scene.patchList.get(i), galerkinState);
            }
        }

        if ( galerkinState.importanceDriven != 0 ) {
            for ( int i = 0; scene.patchList != null && i < scene.patchList.size(); i++ ) {
                GatheringSimpleStrategy.patchUpdatePotential(scene.patchList.get(i));
            }
        }

        return false; // Never done, until we have a better criteria
    }
}
