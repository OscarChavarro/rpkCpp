package vsdk.toolkit.galerkin.processing.visitors;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinIterationMethod;
import vsdk.toolkit.galerkin.GalerkinState;

public final class PowerAccumulatorVisitor implements ClusterLeafVisitor {
    private ColorRgb sourceRadiance;
    private Vector3Dd samplePoint;
    private ColorRgb accumulatedRadiance;

    public PowerAccumulatorVisitor(ColorRgb inSourceRadiance, Vector3Dd inSamplePoint) {
        sourceRadiance = inSourceRadiance;
        samplePoint = inSamplePoint;
        accumulatedRadiance = new ColorRgb();
        accumulatedRadiance = new ColorRgb(0.0, 0.0, 0.0);
    }

    public ColorRgb getAccumulatedRadiance() {
        return accumulatedRadiance;
    }

    /**
Accumulates this leaf contribution towards the visitor sample point.
The contribution is weighted by projected area (cosine term and patch area),
uses the radiance channel that matches the current Galerkin iteration mode,
and ignores intra-cluster visibility.
*/
    @Override
    public void visit(
        GalerkinElement galerkinElement,
        GalerkinState galerkinState)
    {
        Vector3Dd dir = new Vector3Dd();
        dir = samplePoint.subtract(galerkinElement.patch.midPoint());
        double dist = dir.length();
        double srcOs;
        if ( dist < VSDK.EPSILON ) {
            srcOs = 1.0;
        }
        else {
            srcOs = dir.dotProduct(galerkinElement.patch.getNormal()) / dist;
        }
        if ( srcOs <= 0.0 ) {
            return;
        }

        ColorRgb rad;
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL ||
             galerkinState.isJacobi() ) {
            rad = galerkinElement.radiance[0];
        }
        else {
            rad = galerkinElement.unShotRadiance[0];
        }

        accumulatedRadiance = sourceRadiance.addScaled(srcOs * galerkinElement.area, rad);
    }
}
