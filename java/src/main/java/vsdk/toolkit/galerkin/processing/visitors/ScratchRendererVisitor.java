package vsdk.toolkit.galerkin.processing.visitors;

import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.render.sgl.SglContext;
import vsdk.toolkit.skin.Patch;

public final class ScratchRendererVisitor implements ClusterLeafVisitor {
    private Vector3D eyePoint;
    private SglContext sglContext;

    public ScratchRendererVisitor(Vector3D inEyePoint, SglContext inSglContext) {
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
        Vector3D[] v = new Vector3D[4];

        // Backface culling test: only render the element if it is turned towards
        // the current eye point
        if ( patch.normal.dotProduct(eyePoint) + patch.planeConstant < Numeric.EPSILON ) {
            return;
        }

        for ( int i = 0; i < patch.numberOfVertices; i++ ) {
            if ( patch.vertex[i] != null && patch.vertex[i].point != null ) {
                v[i] = new Vector3D(patch.vertex[i].point.x, patch.vertex[i].point.y, patch.vertex[i].point.z);
            }
            else {
                v[i] = new Vector3D();
            }
        }

        if ( sglContext == null ) {
            return;
        }

        sglContext.sglSetGalerkinElement(galerkinElement);
        sglContext.sglPolygon(patch.numberOfVertices, v);
    }
}
