package vsdk.toolkit.galerkin.processing.visitors;

import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinIterationMethod;
import vsdk.toolkit.galerkin.GalerkinState;

public final class PowerAccumulatorVisitor implements ClusterLeafVisitor {
    private ColorRgb sourceRadiance;
    private Vector3D samplePoint;
    private ColorRgb accumulatedRadiance;

    public PowerAccumulatorVisitor(ColorRgb inSourceRadiance, Vector3D inSamplePoint) {
        sourceRadiance = inSourceRadiance;
        samplePoint = inSamplePoint;
        accumulatedRadiance = new ColorRgb();
        accumulatedRadiance.clear();
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
        Vector3D dir = new Vector3D();
        dir.subtraction(samplePoint, galerkinElement.patch.midPoint);
        float dist = dir.norm();
        float srcOs;
        if ( dist < Numeric.EPSILON ) {
            srcOs = 1.0f;
        }
        else {
            srcOs = dir.dotProduct(galerkinElement.patch.normal) / dist;
        }
        if ( srcOs <= 0.0f ) {
            return;
        }

        ColorRgb rad;
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.GAUSS_SEIDEL ||
             galerkinState.galerkinIterationMethod == GalerkinIterationMethod.JACOBI ) {
            rad = galerkinElement.radiance[0];
        }
        else {
            rad = galerkinElement.unShotRadiance[0];
        }

        accumulatedRadiance.addScaled(sourceRadiance, srcOs * galerkinElement.area, rad);
    }
}
