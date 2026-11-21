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
export class BinnedSahHierarchyNode {
    public readonly minimum: number[] = [0, 0, 0];
    public leftOrFirst = 0;
    public readonly maximum: number[] = [0, 0, 0];
    public count = 0;

    public constructor();
    /** Value copy (cpp copies nodes by value) */
    public constructor(other: BinnedSahHierarchyNode);
    public constructor(other?: BinnedSahHierarchyNode) {
        if (other === undefined) {
            return;
        }
        for (let a = 0; a < 3; a++) {
            this.minimum[a] = other.minimum[a]!;
            this.maximum[a] = other.maximum[a]!;
        }
        this.leftOrFirst = other.leftOrFirst;
        this.count = other.count;
    }
}
