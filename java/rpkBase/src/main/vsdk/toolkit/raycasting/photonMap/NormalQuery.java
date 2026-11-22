package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.linealAlgebra.Vector3Df;

public class NormalQuery {
    public IrrPhoton photon;
    public float[] point;
    public Vector3Df normal;
    public float threshold;
    public float maximumDistance;

    public NormalQuery() {
        photon = null;
        point = null;
        normal = new Vector3Df();
        threshold = 0.0f;
        maximumDistance = 0.0f;
    }
}
