import { Ray } from "../common/linealAlgebra/Ray";
import { Statistics } from "../common/statistics/Statistics";
import { Geometry } from "./Geometry";
import { GeometryClassId } from "./GeometryClassId";
import type { RayExclusionSet } from "../environment/geometry/elements/RayExclusionSet";
import type { RayHit } from "../environment/geometry/elements/RayHit";

export class Compound extends Geometry {
  public children: Geometry[] | null;

  public constructor(geometryList: Geometry[] | null) {
    super(GeometryClassId.COMPOUND);
    Statistics.instance().reader.numberOfCompounds++;
    this.children = geometryList;
    Geometry.listBounds(this.children, this.boundingBox);
    this.boundingBox.enlargeTinyBit();
    this.bounded = true;
  }

  public override destroy(): void {
    Statistics.instance().reader.numberOfCompounds--;
    if (this.children !== null) {
      this.children = null;
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

    return Geometry.listDiscretizationIntersect(
      this.children,
      ray,
      minimumDistance,
      maximumDistance,
      hitFlags,
      hitStore,
      exclusions
    );
  }
}
