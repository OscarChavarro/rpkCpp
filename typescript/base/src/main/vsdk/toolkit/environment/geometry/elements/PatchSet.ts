import { Ray } from "../../../common/linealAlgebra/Ray";
import { BoundingBox } from "../../../skin/AxisAlignedBoundingBox";
import { Geometry } from "../../../skin/Geometry";
import { GeometryClassId } from "../../../skin/GeometryClassId";
import type { Patch } from "./Patch";
import type { RayExclusionSet } from "./RayExclusionSet";
import type { RayHit } from "./RayHit";

export class PatchSet extends Geometry {
  private patchList: Patch[] | null;
  private memoryPoolManaged: boolean;

  /**
  A set of the given patches. Without patchBounds the bounds are computed
  from the patches' vertices; with it, it must be the union of the patch
  boxes the caller already has (for instance from the scene's
  FaceBoundsTable)
  */
  public constructor(input: Patch[] | null, patchBounds: BoundingBox | null = null) {
    super(GeometryClassId.PATCH_SET);
    this.memoryPoolManaged = false;
    this.patchList = [];
    for (let i = 0; input !== null && i < input.length; i++) {
      const patch = input[i];
      if (patch !== undefined) {
        this.patchList.push(patch);
      }
    }

    if (patchBounds === null) {
      Geometry.patchListBounds(this.getPatchList(), this.boundingBox);
    }
    else {
      this.boundingBox.enlarge(patchBounds);
    }
    this.boundingBox.enlargeTinyBit();
    this.bounded = true;
  }

  public override destroy(): void {
    if (this.patchList !== null) {
      this.patchList = null;
    }
    super.destroy();
  }

  public override discretizationIntersect(
    ray: Ray,
    minimumDistance: number,
    maximumDistance: number[],
    hitFlags: number,
    hitStore: RayHit | null,
    exclusions: RayExclusionSet
  ): RayHit | null {
    if (!this.discretizationIntersectPreTest(ray, minimumDistance, maximumDistance, exclusions)) {
      return null;
    }

    return Geometry.patchListIntersect(this.patchList, ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
  }

  public getPatchList(): Patch[] | null {
    return this.patchList;
  }

  public isMemoryPoolManaged(): boolean {
    return this.memoryPoolManaged;
  }

  public setMemoryPoolManaged(value: boolean): void {
    this.memoryPoolManaged = value;
  }
}
