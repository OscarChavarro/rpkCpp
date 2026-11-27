package vsdk.toolkit.scene;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

public class Plane {
    public Vector3Dd normal;
    public double d;

    public Plane() {
        normal = new Vector3Dd();
        d = 0.0;
    }
}
