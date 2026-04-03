package vsdk.toolkit.skin;

import vsdk.toolkit.common.linealAlgebra.Vector3D;

public interface Patch {
    void uv(Vector3D point, double[] u, double[] v);

    Vector3D textureCoordAtUv(double u, double v);

    void interpolatedFrameAtUv(double u, double v, Vector3D x, Vector3D y, Vector3D z);
}
