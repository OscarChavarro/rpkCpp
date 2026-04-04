package vsdk.toolkit.galerkin.processing;

import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.scene.Scene;

public final class GatheringClusteredStrategy extends GatheringStrategy {
    private static float updatePotential(GalerkinElement cluster) {
        return 0.0f;
    }

    private static void updateClusterDirectPotential(GalerkinElement element, float potentialIncrement) {
    }

    public GatheringClusteredStrategy() {
    }

    @Override
    public boolean doGatheringIteration(Scene scene, GalerkinState galerkinState, RenderOptions renderOptions) {
        return true;
    }
}
