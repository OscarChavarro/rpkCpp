package vsdk.toolkit.scene;

import vsdk.toolkit.common.linealAlgebra.Vector3D;

public class Plane {
    public Vector3D normal;
    public float d;

    public Plane() {
        normal = new Vector3D();
        d = 0.0f;
    }
}
