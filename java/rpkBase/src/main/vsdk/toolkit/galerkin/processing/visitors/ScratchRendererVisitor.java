package vsdk.toolkit.galerkin.processing.visitors;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.render.sgl.SglContext;
import vsdk.toolkit.environment.geometry.elements.Patch;

public final class ScratchRendererVisitor implements ClusterLeafVisitor {
    private Vector3Df eyePoint;
    private SglContext sglContext;

    public ScratchRendererVisitor(Vector3Df inEyePoint, SglContext inSglContext) {
        eyePoint = inEyePoint;
        sglContext = inSglContext;
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
        Vector3Df[] v = new Vector3Df[4];

        // Backface culling test: only render the element if it is turned towards
        // the current eye point
        if ( patch.getNormal().dotProduct(eyePoint) + patch.planeConstant < VSDK.EPSILON ) {
            return;
        }

        for ( int i = 0; i < patch.numberOfVertices; i++ ) {
            if ( patch.vertex[i] != null && patch.vertex[i].point != null ) {
                v[i] = new Vector3Df(patch.vertex[i].point.x(), patch.vertex[i].point.y(), patch.vertex[i].point.z());
            }
            else {
                v[i] = new Vector3Df();
            }
        }

        if ( sglContext == null ) {
            return;
        }

        sglContext.sglSetGalerkinElement(galerkinElement);
        sglContext.sglPolygon(patch.numberOfVertices, v);
    }
}
