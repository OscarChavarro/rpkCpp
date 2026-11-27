package vsdk.toolkit.galerkin.processing.visitors;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.galerkin.GalerkinElementVisibilityBuffer;
import vsdk.toolkit.environment.geometry.elements.Patch;

public final class ScratchRendererVisitor implements ClusterLeafVisitor {
    private final Vector3Dd eyePoint;
    private final GalerkinElementVisibilityBuffer target;

    public ScratchRendererVisitor(Vector3Dd inEyePoint, GalerkinElementVisibilityBuffer inTarget) {
        eyePoint = inEyePoint;
        target = inTarget;
    }

    @Override
    public void visit(
        GalerkinElement galerkinElement,
        GalerkinState galerkinState)
    {
        Patch patch = galerkinElement.patch;
        if ( patch == null ) {
            return;
        }
        Vector3Dd[] v = new Vector3Dd[4];

        // Backface culling test: only render the element if it is turned towards
        // the current eye point
        if ( patch.getNormal().dotProduct(eyePoint) + patch.getPlaneConstant() < VSDK.EPSILON ) {
            return;
        }

        for ( int i = 0; i < patch.getNumberOfVertices(); i++ ) {
            if ( patch.getVertices()[i] != null && patch.getVertices()[i].point != null ) {
                v[i] = new Vector3Dd(patch.getVertices()[i].point.x(), patch.getVertices()[i].point.y(), patch.getVertices()[i].point.z());
            }
            else {
                v[i] = new Vector3Dd();
            }
        }

        target.drawPolygon(patch.getNumberOfVertices(), v, galerkinElement);
    }
}
