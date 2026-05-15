package vsdk.toolkit.raycasting.stochasticRaytracing;

/**
A full path, basically an array of 'numberOfNodes' path nodes
*/
public class Path {
    public int numberOfNodes;
    public int nodesAllocated;
    public StochasticRaytracingPathNode[] nodes;

    public Path() {
        numberOfNodes = 0;
        nodesAllocated = 0;
        nodes = null;
    }
}
