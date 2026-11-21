package vsdk.toolkit.environment.geometry.boundingVolumeHierarchy;

/**
Leaf test of `BinnedSahHierarchy.traverse`: as `BinnedSahLeafVisitor`, and
it may end the traversal by returning true (any-hit queries).
*/
public interface BinnedSahStoppingLeafVisitor {
    boolean visit(int primitive, double[] inOutBestT);
}
