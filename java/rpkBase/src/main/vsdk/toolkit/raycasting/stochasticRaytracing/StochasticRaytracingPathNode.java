package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.environment.geometry.elements.Patch;

/**
Path node: contains all necessary data for computing the score afterwards
*/
public class StochasticRaytracingPathNode {
    public Patch patch;
    public double probability;
    public Vector3Df inPoint;
    public Vector3Df outpoint;

    public StochasticRaytracingPathNode() {
        patch = null;
        probability = 0.0;
        inPoint = new Vector3Df();
        outpoint = new Vector3Df();
    }
}
