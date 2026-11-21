package vsdk.toolkit.environment.geometry.boundingVolumeHierarchy;

/**
Leaf test of `BinnedSahHierarchy.traverseClosest`: tests one primitive and
lowers `inOutBestT[0]` when it finds a closer hit (or applies its own tie
rule). The traversal only skips nodes whose entry is beyond `inOutBestT[0]`.
*/
public interface BinnedSahLeafVisitor {
    void visit(int primitive, double[] inOutBestT);
}
