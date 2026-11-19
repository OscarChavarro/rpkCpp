package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.linealAlgebra.Vector3D;

public class NormalQuery {
    public IrrPhoton photon;
    public float[] point;
    public Vector3D normal;
    public float threshold;
    public float maximumDistance;

    public NormalQuery() {
        photon = null;
        point = null;
        normal = new Vector3D();
        threshold = 0.0f;
        maximumDistance = 0.0f;
    }
}
