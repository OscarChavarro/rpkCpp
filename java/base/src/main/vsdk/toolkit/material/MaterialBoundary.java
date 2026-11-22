package vsdk.toolkit.material;

import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;

/**
Conversions between RenderPark's single precision Vector3D and VITRAL's
Vector3Df at the material boundary: the same three floats, so a value
crosses unchanged.
*/
public final class MaterialBoundary {
    private MaterialBoundary() {
    }

    public static Vector3Df toVitral(Vector3D v) {
        return new Vector3Df(v.x, v.y, v.z);
    }

    public static Vector3D fromVitral(Vector3Df v) {
        return new Vector3D(v.x(), v.y(), v.z());
    }
}
