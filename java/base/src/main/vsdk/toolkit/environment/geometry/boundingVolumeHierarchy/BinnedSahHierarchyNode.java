package vsdk.toolkit.environment.geometry.boundingVolumeHierarchy;

/**
One node of a `BinnedSahHierarchy`, an element of the flat binary tree
contract of `doc/flatBinaryTreeContract.md` (the Vulkan 1.4 compute path
copies it into its single precision `VulkanBvhNode`, `CasNode` of
`computeAccelerationStructure.glsl`):

- inner node (`count == 0`): children at `leftOrFirst` and `leftOrFirst + 1`;
- leaf (`count > 0`): entries `leftOrFirst` .. `leftOrFirst + count - 1`
  of the primitive order.

As in [WALD2007].3.6: the child pair of an inner node is stored next to
each other, a leaf refers to one contiguous block of the primitive ids.
*/
public final class BinnedSahHierarchyNode {
    public final double[] minimum = new double[3];
    public int leftOrFirst;
    public final double[] maximum = new double[3];
    public int count;
}
