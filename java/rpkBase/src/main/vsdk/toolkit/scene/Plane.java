package vsdk.toolkit.scene;

import vsdk.toolkit.common.linealAlgebra.Vector3Df;

public class Plane {
    public Vector3Df normal;
    public float d;

    public Plane() {
        normal = new Vector3Df();
        d = 0.0f;
    }
}
