import { BoundingBox } from "../skin/AxisAlignedBoundingBox";
import type { Patch } from "../environment/geometry/elements/Patch";

/**
Conservative bounds of every face of a scene, in one contiguous array indexed
by the dense face index (Patch.getFaceIndex()). Built once by the scene after
the face indices are assigned, from Patch.computeBounds, and read-only
afterwards: the voxel grid, the scene query and shaft culling (its patch
tests and the bounds of the patch sets it creates) read it.
*/
export class FaceBoundsTable {
  private readonly bounds: BoundingBox[];
  private readonly numberOfFaces: number;

  public constructor(faces: Patch[], inNumberOfFaces: number) {
    this.numberOfFaces = inNumberOfFaces;
    this.bounds = [];
    for (let i = 0; i < this.numberOfFaces; i++) {
      const box = new BoundingBox();
      faces[i]!.computeBounds(box);
      this.bounds.push(box);
    }
  }

  public getNumberOfFaces(): number {
    return this.numberOfFaces;
  }

  public get(faceIndex: number): BoundingBox {
    return this.bounds[faceIndex]!;
  }
}
