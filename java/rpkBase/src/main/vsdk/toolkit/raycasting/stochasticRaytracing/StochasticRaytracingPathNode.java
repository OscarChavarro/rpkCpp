package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.elements.Patch;

/**
Path node: contains all necessary data for computing the score afterwards
*/
public class StochasticRaytracingPathNode {
    public Patch patch;
    public double probability;
    public Vector3Dd inPoint;
    public Vector3Dd outpoint;

    public StochasticRaytracingPathNode() {
        patch = null;
        probability = 0.0;
        inPoint = new Vector3Dd();
        outpoint = new Vector3Dd();
    }
}
