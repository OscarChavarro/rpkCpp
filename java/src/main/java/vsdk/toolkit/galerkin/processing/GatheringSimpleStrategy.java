package vsdk.toolkit.galerkin.processing;

import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.skin.Patch;

public final class GatheringSimpleStrategy extends GatheringStrategy {
    private static void patchUpdatePotential(Patch patch) {
    }

    private static void patchUpdateRadiance(Patch patch, GalerkinState galerkinState) {
    }

    private static void patchLazyCreateInteractions(
        Scene scene,
        Patch patch,
        GalerkinState galerkinState)
    {
    }

    private static void patchGather(
        Patch patch,
        Scene scene,
        GalerkinState galerkinState)
    {
    }

    public GatheringSimpleStrategy() {
    }

    @Override
    public boolean doGatheringIteration(Scene scene, GalerkinState galerkinState, RenderOptions renderOptions) {
        return true;
    }
}
