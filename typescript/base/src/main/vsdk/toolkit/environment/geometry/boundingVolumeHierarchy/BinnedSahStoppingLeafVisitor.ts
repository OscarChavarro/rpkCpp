/**
Leaf test of `BinnedSahHierarchy.traverse`: as `BinnedSahLeafVisitor`, and
it may end the traversal by returning true (any-hit queries).
*/
export interface BinnedSahStoppingLeafVisitor {
    visit(primitive: number, inOutBestT: number[]): boolean;
}
