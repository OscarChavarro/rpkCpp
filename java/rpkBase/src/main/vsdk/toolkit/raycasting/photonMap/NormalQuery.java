package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

public class NormalQuery {
    public IrrPhoton photon;
    public double[] point;
    public Vector3Dd normal;
    public double threshold;
    public double maximumDistance;

    public NormalQuery() {
        photon = null;
        point = null;
        normal = new Vector3Dd();
        threshold = 0.0;
        maximumDistance = 0.0;
    }
}
