package vsdk.toolkit.galerkin.processing.visitors;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinIterationMethod;
import vsdk.toolkit.galerkin.GalerkinState;

public final class PowerAccumulatorVisitor implements ClusterLeafVisitor {
    private ColorRgbMutable sourceRadiance;
    private Vector3Df samplePoint;
    private ColorRgbMutable accumulatedRadiance;

    public PowerAccumulatorVisitor(ColorRgbMutable inSourceRadiance, Vector3Df inSamplePoint) {
        sourceRadiance = inSourceRadiance;
        samplePoint = inSamplePoint;
        accumulatedRadiance = new ColorRgbMutable();
        accumulatedRadiance.clear();
    }

    public ColorRgbMutable getAccumulatedRadiance() {
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
        Vector3Df dir = new Vector3Df();
        dir = samplePoint.subtract(galerkinElement.patch.midPoint());
        float dist = dir.length();
        float srcOs;
        if ( dist < VSDK.EPSILON ) {
            srcOs = 1.0f;
        }
        else {
            srcOs = dir.dotProduct(galerkinElement.patch.getNormal()) / dist;
        }
        if ( srcOs <= 0.0f ) {
            return;
        }

        ColorRgbMutable rad;
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL ||
             galerkinState.isJacobi() ) {
            rad = galerkinElement.radiance[0];
        }
        else {
            rad = galerkinElement.unShotRadiance[0];
        }

        accumulatedRadiance.addScaled(sourceRadiance, srcOs * galerkinElement.area, rad);
    }
}
