import { Ray } from "../../../common/linealAlgebra/Ray";
import { Geometry } from "../../../skin/Geometry";
import { GeometryClassId } from "../../../skin/GeometryClassId";
import type { Patch } from "./Patch";
import type { RayHit } from "./RayHit";

export class PatchSet extends Geometry {
  private patchList: Patch[] | null;
  private memoryPoolManaged: boolean;

  public constructor(input: Patch[] | null) {
    super(GeometryClassId.PATCH_SET);
    this.memoryPoolManaged = false;
    this.patchList = [];
    for (let i = 0; input !== null && i < input.length; i++) {
      const patch = input[i];
      if (patch !== undefined) {
        this.patchList.push(patch);
      }
    }

    Geometry.patchListBounds(this.getPatchList(), this.boundingBox);
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
    hitStore: RayHit | null
  ): RayHit | null {
    if (!this.discretizationIntersectPreTest(ray, minimumDistance, maximumDistance)) {
      return null;
    }

    return Geometry.patchListIntersect(this.patchList, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
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
