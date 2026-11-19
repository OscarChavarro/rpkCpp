package vsdk.toolkit.galerkin.processing.visitors;

import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinIterationMethod;
import vsdk.toolkit.galerkin.GalerkinState;

public final class MaximumRadianceVisitor implements ClusterLeafVisitor {
    private ColorRgbMutable accumulatedRadiance;

    public MaximumRadianceVisitor() {
        accumulatedRadiance = new ColorRgbMutable();
        accumulatedRadiance.clear();
    }

    public ColorRgbMutable getAccumulatedRadiance() {
        return accumulatedRadiance;
    }

    @Override
    public void visit(
        GalerkinElement galerkinElement,
        GalerkinState galerkinState)
    {
        ColorRgbMutable rad;
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL ||
             galerkinState.galerkinIterationMethod == GalerkinIterationMethod.JACOBI ) {
            rad = galerkinElement.radiance[0];
        }
        else {
            rad = galerkinElement.unShotRadiance[0];
        }
        accumulatedRadiance.maximum(accumulatedRadiance, rad);
    }
}
