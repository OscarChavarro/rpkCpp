import { Numeric } from "../common/linealAlgebra/Numeric";
import { Ray } from "../common/linealAlgebra/Ray";
import { BoundingBox } from "./AxisAlignedBoundingBox";
import { BoundingBoxCoordinateIndex } from "./BoundingBoxCoordinateIndex";

export class MinMaxBox {
  private readonly boundingBox: BoundingBox;

  public constructor(sourceBoundingBox: BoundingBox | null) {
    this.boundingBox = new BoundingBox();
    if (sourceBoundingBox !== null) {
      this.boundingBox.copyFrom(sourceBoundingBox);
    }
  }

  public updateFromBoundingBox(sourceBoundingBox: BoundingBox | null): void {
    if (sourceBoundingBox === null) {
      return;
    }
    this.boundingBox.copyFrom(sourceBoundingBox);
  }

  private static clipAxisSlab(
    minimumBound: number,
    maximumBound: number,
    origin: number,
    direction: number,
    toleranceScale: number,
    nearDistance: number[],
    farDistance: number[]
  ): boolean {
    if (direction === 0.0) {
      return !(origin < minimumBound || origin > maximumBound);
    }

    const invDirection = 1.0 / direction;
    let entryDistance = (minimumBound - origin) * invDirection;
    let exitDistance = (maximumBound - origin) * invDirection;
    if (entryDistance > exitDistance) {
      const swapValue = entryDistance;
      entryDistance = exitDistance;
      exitDistance = swapValue;
    }

    if (exitDistance < nearDistance[0]!) {
      return false;
    }
    if (entryDistance > nearDistance[0]!) {
      nearDistance[0]! = entryDistance;
    }
    if (exitDistance < farDistance[0]!) {
      farDistance[0]! = exitDistance;
    }
    return nearDistance[0]! <= (farDistance[0]! * toleranceScale);
  }

  public intersectingSegment(ray: Ray | null, tMin: number[] | null, tMax: number[] | null): boolean {
    if (ray === null || tMin === null || tMin.length === 0 || tMax === null || tMax.length === 0) {
      return false;
    }

    const minimumDistance = tMin[0]!;
    const maximumDistance = tMax[0]!;
    const box = this.boundingBox.rawCoordinates();
    const nearDistance = [minimumDistance];
    const farDistance = [maximumDistance];
    const toleranceScale = 1.0 + Numeric.EPSILON_FLOAT;

    if (!MinMaxBox.clipAxisSlab(
      box[BoundingBoxCoordinateIndex.MIN_X]!, box[BoundingBoxCoordinateIndex.MAX_X]!,
      ray.position.x, ray.direction.x,
      toleranceScale,
      nearDistance, farDistance
    )) {
      return false;
    }
    if (!MinMaxBox.clipAxisSlab(
      box[BoundingBoxCoordinateIndex.MIN_Y]!, box[BoundingBoxCoordinateIndex.MAX_Y]!,
      ray.position.y, ray.direction.y,
      toleranceScale,
      nearDistance, farDistance
    )) {
      return false;
    }
    if (!MinMaxBox.clipAxisSlab(
      box[BoundingBoxCoordinateIndex.MIN_Z]!, box[BoundingBoxCoordinateIndex.MAX_Z]!,
      ray.position.z, ray.direction.z,
      toleranceScale,
      nearDistance, farDistance
    )) {
      return false;
    }

    tMin[0] = nearDistance[0]!;
    tMax[0] = farDistance[0]!;

    if (nearDistance[0]! === minimumDistance) {
      return farDistance[0]! < maximumDistance;
    }
    return nearDistance[0]! < maximumDistance;
  }

  public intersect(ray: Ray, minimumDistance: number, maximumDistance: number[] | null): boolean {
    if (maximumDistance === null || maximumDistance.length === 0) {
      return false;
    }

    const tMin = [minimumDistance];
    const tMax = [maximumDistance[0]!];
    const hit = this.intersectingSegment(ray, tMin, tMax);
    if (hit) {
      if (tMin[0]! === minimumDistance) {
        if (tMax[0]! < maximumDistance[0]!) {
          maximumDistance[0] = tMax[0]!;
        }
      }
      else if (tMin[0]! < maximumDistance[0]!) {
        maximumDistance[0] = tMin[0]!;
      }
    }
    return hit;
  }
}
