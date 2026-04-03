package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.skin.Patch;

/**
Path node: contains all necessary data for computing the score afterwards
*/
public class StochasticRaytracingPathNode {
    public Patch patch;
    public double probability;
    public Vector3D inPoint;
    public Vector3D outpoint;

    public StochasticRaytracingPathNode() {
        patch = null;
        probability = 0.0;
        inPoint = new Vector3D();
        outpoint = new Vector3D();
    }
}
