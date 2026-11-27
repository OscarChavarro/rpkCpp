package vsdk.toolkit.galerkin.processing.visitors;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinIterationMethod;
import vsdk.toolkit.galerkin.GalerkinState;

public final class MaximumRadianceVisitor implements ClusterLeafVisitor {
    private ColorRgb accumulatedRadiance;

    public MaximumRadianceVisitor() {
        accumulatedRadiance = new ColorRgb();
        accumulatedRadiance = new ColorRgb(0.0, 0.0, 0.0);
    }

    public ColorRgb getAccumulatedRadiance() {
        return accumulatedRadiance;
    }

    @Override
    public void visit(
        GalerkinElement galerkinElement,
        GalerkinState galerkinState)
    {
        ColorRgb rad;
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL ||
             galerkinState.isJacobi() ) {
            rad = galerkinElement.radiance[0];
        }
        else {
            rad = galerkinElement.unShotRadiance[0];
        }
        accumulatedRadiance = accumulatedRadiance.maximum(rad);
    }
}
