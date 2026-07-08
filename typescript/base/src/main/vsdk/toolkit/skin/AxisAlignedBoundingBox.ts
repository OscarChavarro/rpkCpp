import { Numeric } from "../common/linealAlgebra/Numeric";
import { Ray } from "../common/linealAlgebra/Ray";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { BoundingBoxCoordinateIndex } from "./BoundingBoxCoordinateIndex";

type BoundingCoords = [number, number, number, number, number, number];
type CoordIndex = 0 | 1 | 2 | 3 | 4 | 5;

export class BoundingBox {
  private readonly coordinates: BoundingCoords;

  private static setIfLess(a: BoundingCoords, idx: CoordIndex, b: number): void {
    a[idx] = a[idx] < b ? a[idx] : b;
  }

  private static setIfGreater(a: BoundingCoords, idx: CoordIndex, b: number): void {
    a[idx] = a[idx] > b ? a[idx] : b;
  }

  public constructor() {
    this.coordinates = [0, 0, 0, 0, 0, 0];
    this.coordinates[BoundingBoxCoordinateIndex.MIN_X] = Numeric.HUGE_FLOAT_VALUE;
    this.coordinates[BoundingBoxCoordinateIndex.MIN_Y] = Numeric.HUGE_FLOAT_VALUE;
    this.coordinates[BoundingBoxCoordinateIndex.MIN_Z] = Numeric.HUGE_FLOAT_VALUE;
    this.coordinates[BoundingBoxCoordinateIndex.MAX_X] = -Numeric.HUGE_FLOAT_VALUE;
    this.coordinates[BoundingBoxCoordinateIndex.MAX_Y] = -Numeric.HUGE_FLOAT_VALUE;
    this.coordinates[BoundingBoxCoordinateIndex.MAX_Z] = -Numeric.HUGE_FLOAT_VALUE;
  }

  public maxExtent(): number {
    const dx = this.coordinates[BoundingBoxCoordinateIndex.MAX_X] - this.coordinates[BoundingBoxCoordinateIndex.MIN_X];
    const dy = this.coordinates[BoundingBoxCoordinateIndex.MAX_Y] - this.coordinates[BoundingBoxCoordinateIndex.MIN_Y];
    const dz = this.coordinates[BoundingBoxCoordinateIndex.MAX_Z] - this.coordinates[BoundingBoxCoordinateIndex.MIN_Z];
    return dx > dy ? (dx > dz ? dx : dz) : (dy > dz ? dy : dz);
  }

  public outOfBounds(p: Vector3D): boolean {
    return p.x < this.coordinates[BoundingBoxCoordinateIndex.MIN_X] || p.x > this.coordinates[BoundingBoxCoordinateIndex.MAX_X]
      || p.y < this.coordinates[BoundingBoxCoordinateIndex.MIN_Y] || p.y > this.coordinates[BoundingBoxCoordinateIndex.MAX_Y]
      || p.z < this.coordinates[BoundingBoxCoordinateIndex.MIN_Z] || p.z > this.coordinates[BoundingBoxCoordinateIndex.MAX_Z];
  }

  public center(): Vector3D {
    return new Vector3D(
      0.5 * (this.coordinates[BoundingBoxCoordinateIndex.MIN_X] + this.coordinates[BoundingBoxCoordinateIndex.MAX_X]),
      0.5 * (this.coordinates[BoundingBoxCoordinateIndex.MIN_Y] + this.coordinates[BoundingBoxCoordinateIndex.MAX_Y]),
      0.5 * (this.coordinates[BoundingBoxCoordinateIndex.MIN_Z] + this.coordinates[BoundingBoxCoordinateIndex.MAX_Z])
    );
  }

  public setAsUnion(a: BoundingBox, b: BoundingBox): void {
    const aCoords = a.rawCoordinates();
    const bCoords = b.rawCoordinates();
    for (let i = BoundingBoxCoordinateIndex.MIN_X; i <= BoundingBoxCoordinateIndex.MIN_Z; i++) {
      const idx = i as CoordIndex;
      this.coordinates[idx] = aCoords[idx] < bCoords[idx] ? aCoords[idx] : bCoords[idx];
    }

    for (let i = BoundingBoxCoordinateIndex.MAX_X; i <= BoundingBoxCoordinateIndex.MAX_Z; i++) {
      const idx = i as CoordIndex;
      this.coordinates[idx] = aCoords[idx] > bCoords[idx] ? aCoords[idx] : bCoords[idx];
    }
  }

  public disjointToOtherBoundingBox(other: BoundingBox): boolean {
    const otherCoords = other.rawCoordinates();
    return (this.coordinates[BoundingBoxCoordinateIndex.MIN_X] > otherCoords[BoundingBoxCoordinateIndex.MAX_X])
      || (otherCoords[BoundingBoxCoordinateIndex.MIN_X] > this.coordinates[BoundingBoxCoordinateIndex.MAX_X])
      || (this.coordinates[BoundingBoxCoordinateIndex.MIN_Y] > otherCoords[BoundingBoxCoordinateIndex.MAX_Y])
      || (otherCoords[BoundingBoxCoordinateIndex.MIN_Y] > this.coordinates[BoundingBoxCoordinateIndex.MAX_Y])
      || (this.coordinates[BoundingBoxCoordinateIndex.MIN_Z] > otherCoords[BoundingBoxCoordinateIndex.MAX_Z])
      || (otherCoords[BoundingBoxCoordinateIndex.MIN_Z] > this.coordinates[BoundingBoxCoordinateIndex.MAX_Z]);
  }

  public behindPlane(normal: Vector3D, distance: number): boolean {
    const p = new Vector3D();

    p.x = normal.x > 0.0 ? this.coordinates[BoundingBoxCoordinateIndex.MAX_X] : this.coordinates[BoundingBoxCoordinateIndex.MIN_X];
    p.y = normal.y > 0.0 ? this.coordinates[BoundingBoxCoordinateIndex.MAX_Y] : this.coordinates[BoundingBoxCoordinateIndex.MIN_Y];
    p.z = normal.z > 0.0 ? this.coordinates[BoundingBoxCoordinateIndex.MAX_Z] : this.coordinates[BoundingBoxCoordinateIndex.MIN_Z];

    return normal.dotProduct(p) + distance <= 0.0;
  }

  public copyFrom(other: BoundingBox): void {
    const otherCoords = other.rawCoordinates();
    this.coordinates[BoundingBoxCoordinateIndex.MIN_X] = otherCoords[BoundingBoxCoordinateIndex.MIN_X];
    this.coordinates[BoundingBoxCoordinateIndex.MIN_Y] = otherCoords[BoundingBoxCoordinateIndex.MIN_Y];
    this.coordinates[BoundingBoxCoordinateIndex.MIN_Z] = otherCoords[BoundingBoxCoordinateIndex.MIN_Z];
    this.coordinates[BoundingBoxCoordinateIndex.MAX_X] = otherCoords[BoundingBoxCoordinateIndex.MAX_X];
    this.coordinates[BoundingBoxCoordinateIndex.MAX_Y] = otherCoords[BoundingBoxCoordinateIndex.MAX_Y];
    this.coordinates[BoundingBoxCoordinateIndex.MAX_Z] = otherCoords[BoundingBoxCoordinateIndex.MAX_Z];
  }

  public enlarge(other: BoundingBox): void {
    const otherCoords = other.rawCoordinates();
    BoundingBox.setIfLess(this.coordinates, BoundingBoxCoordinateIndex.MIN_X, otherCoords[BoundingBoxCoordinateIndex.MIN_X]);
    BoundingBox.setIfLess(this.coordinates, BoundingBoxCoordinateIndex.MIN_Y, otherCoords[BoundingBoxCoordinateIndex.MIN_Y]);
    BoundingBox.setIfLess(this.coordinates, BoundingBoxCoordinateIndex.MIN_Z, otherCoords[BoundingBoxCoordinateIndex.MIN_Z]);
    BoundingBox.setIfGreater(this.coordinates, BoundingBoxCoordinateIndex.MAX_X, otherCoords[BoundingBoxCoordinateIndex.MAX_X]);
    BoundingBox.setIfGreater(this.coordinates, BoundingBoxCoordinateIndex.MAX_Y, otherCoords[BoundingBoxCoordinateIndex.MAX_Y]);
    BoundingBox.setIfGreater(this.coordinates, BoundingBoxCoordinateIndex.MAX_Z, otherCoords[BoundingBoxCoordinateIndex.MAX_Z]);
  }

  public enlargeToIncludePoint(point: Vector3D): void {
    BoundingBox.setIfLess(this.coordinates, BoundingBoxCoordinateIndex.MIN_X, point.x);
    BoundingBox.setIfLess(this.coordinates, BoundingBoxCoordinateIndex.MIN_Y, point.y);
    BoundingBox.setIfLess(this.coordinates, BoundingBoxCoordinateIndex.MIN_Z, point.z);
    BoundingBox.setIfGreater(this.coordinates, BoundingBoxCoordinateIndex.MAX_X, point.x);
    BoundingBox.setIfGreater(this.coordinates, BoundingBoxCoordinateIndex.MAX_Y, point.y);
    BoundingBox.setIfGreater(this.coordinates, BoundingBoxCoordinateIndex.MAX_Z, point.z);
  }

  public enlargeTinyBit(): void {
    let dx = (this.coordinates[BoundingBoxCoordinateIndex.MAX_X] - this.coordinates[BoundingBoxCoordinateIndex.MIN_X]) * 1e-4;
    let dy = (this.coordinates[BoundingBoxCoordinateIndex.MAX_Y] - this.coordinates[BoundingBoxCoordinateIndex.MIN_Y]) * 1e-4;
    let dz = (this.coordinates[BoundingBoxCoordinateIndex.MAX_Z] - this.coordinates[BoundingBoxCoordinateIndex.MIN_Z]) * 1e-4;

    if (dx < Numeric.EPSILON_FLOAT) {
      dx = Numeric.EPSILON_FLOAT;
    }
    if (dy < Numeric.EPSILON_FLOAT) {
      dy = Numeric.EPSILON_FLOAT;
    }
    if (dz < Numeric.EPSILON_FLOAT) {
      dz = Numeric.EPSILON_FLOAT;
    }

    this.coordinates[BoundingBoxCoordinateIndex.MIN_X] -= dx;
    this.coordinates[BoundingBoxCoordinateIndex.MAX_X] += dx;
    this.coordinates[BoundingBoxCoordinateIndex.MIN_Y] -= dy;
    this.coordinates[BoundingBoxCoordinateIndex.MAX_Y] += dy;
    this.coordinates[BoundingBoxCoordinateIndex.MIN_Z] -= dz;
    this.coordinates[BoundingBoxCoordinateIndex.MAX_Z] += dz;
  }

  public computeContributionFlags(other: BoundingBox, hasMinMax1: boolean[], hasMinMax2: boolean[]): void {
    const otherCoords = other.rawCoordinates();
    for (let i = BoundingBoxCoordinateIndex.MIN_X; i <= BoundingBoxCoordinateIndex.MIN_Z; i++) {
      const idx = i as CoordIndex;
      if (this.coordinates[idx] < otherCoords[idx]) {
        hasMinMax1[idx] = true;
      }
      else if (!Numeric.doubleEqual(this.coordinates[idx], otherCoords[idx], Numeric.EPSILON)) {
        hasMinMax2[idx] = true;
      }
    }

    for (let i = BoundingBoxCoordinateIndex.MAX_X; i <= BoundingBoxCoordinateIndex.MAX_Z; i++) {
      const idx = i as CoordIndex;
      if (this.coordinates[idx] > otherCoords[idx]) {
        hasMinMax1[idx] = true;
      }
      else if (!Numeric.doubleEqual(this.coordinates[idx], otherCoords[idx], Numeric.EPSILON)) {
        hasMinMax2[idx] = true;
      }
    }
  }

  public dx(): number {
    return this.coordinates[BoundingBoxCoordinateIndex.MAX_X] - this.coordinates[BoundingBoxCoordinateIndex.MIN_X];
  }

  public dy(): number {
    return this.coordinates[BoundingBoxCoordinateIndex.MAX_Y] - this.coordinates[BoundingBoxCoordinateIndex.MIN_Y];
  }

  public dz(): number {
    return this.coordinates[BoundingBoxCoordinateIndex.MAX_Z] - this.coordinates[BoundingBoxCoordinateIndex.MIN_Z];
  }

  public voxelSize(nx: number, ny: number, nz: number): Vector3D {
    return new Vector3D(
      this.dx() / nx,
      this.dy() / ny,
      this.dz() / nz
    );
  }

  public enlargeByFactor(factor: number): void {
    let fdx = this.dx() * factor;
    let fdy = this.dy() * factor;
    let fdz = this.dz() * factor;

    if (fdx < Numeric.EPSILON_FLOAT) {
      fdx = Numeric.EPSILON_FLOAT;
    }
    if (fdy < Numeric.EPSILON_FLOAT) {
      fdy = Numeric.EPSILON_FLOAT;
    }
    if (fdz < Numeric.EPSILON_FLOAT) {
      fdz = Numeric.EPSILON_FLOAT;
    }

    this.coordinates[BoundingBoxCoordinateIndex.MIN_X] -= fdx;
    this.coordinates[BoundingBoxCoordinateIndex.MAX_X] += fdx;
    this.coordinates[BoundingBoxCoordinateIndex.MIN_Y] -= fdy;
    this.coordinates[BoundingBoxCoordinateIndex.MAX_Y] += fdy;
    this.coordinates[BoundingBoxCoordinateIndex.MIN_Z] -= fdz;
    this.coordinates[BoundingBoxCoordinateIndex.MAX_Z] += fdz;
  }

  public minPoint(): Vector3D {
    return new Vector3D(
      this.coordinates[BoundingBoxCoordinateIndex.MIN_X],
      this.coordinates[BoundingBoxCoordinateIndex.MIN_Y],
      this.coordinates[BoundingBoxCoordinateIndex.MIN_Z]
    );
  }

  public maxPoint(): Vector3D {
    return new Vector3D(
      this.coordinates[BoundingBoxCoordinateIndex.MAX_X],
      this.coordinates[BoundingBoxCoordinateIndex.MAX_Y],
      this.coordinates[BoundingBoxCoordinateIndex.MAX_Z]
    );
  }

  public minX(): number {
    return this.coordinates[BoundingBoxCoordinateIndex.MIN_X];
  }

  public minY(): number {
    return this.coordinates[BoundingBoxCoordinateIndex.MIN_Y];
  }

  public minZ(): number {
    return this.coordinates[BoundingBoxCoordinateIndex.MIN_Z];
  }

  public maxX(): number {
    return this.coordinates[BoundingBoxCoordinateIndex.MAX_X];
  }

  public maxY(): number {
    return this.coordinates[BoundingBoxCoordinateIndex.MAX_Y];
  }

  public maxZ(): number {
    return this.coordinates[BoundingBoxCoordinateIndex.MAX_Z];
  }

  public rawCoordinates(): BoundingCoords {
    return this.coordinates;
  }

  public valueAt(idx: number): number {
    switch (idx) {
      case BoundingBoxCoordinateIndex.MIN_X: return this.minX();
      case BoundingBoxCoordinateIndex.MIN_Y: return this.minY();
      case BoundingBoxCoordinateIndex.MIN_Z: return this.minZ();
      case BoundingBoxCoordinateIndex.MAX_X: return this.maxX();
      case BoundingBoxCoordinateIndex.MAX_Y: return this.maxY();
      case BoundingBoxCoordinateIndex.MAX_Z: return this.maxZ();
      default: return 0.0;
    }
  }

  public corners(out: Vector3D[]): void {
    const minX = this.coordinates[BoundingBoxCoordinateIndex.MIN_X];
    const minY = this.coordinates[BoundingBoxCoordinateIndex.MIN_Y];
    const minZ = this.coordinates[BoundingBoxCoordinateIndex.MIN_Z];
    const maxX = this.coordinates[BoundingBoxCoordinateIndex.MAX_X];
    const maxY = this.coordinates[BoundingBoxCoordinateIndex.MAX_Y];
    const maxZ = this.coordinates[BoundingBoxCoordinateIndex.MAX_Z];

    out[0]?.set(minX, minY, minZ);
    out[1]?.set(maxX, minY, minZ);
    out[2]?.set(minX, maxY, minZ);
    out[3]?.set(maxX, maxY, minZ);
    out[4]?.set(minX, minY, maxZ);
    out[5]?.set(maxX, minY, maxZ);
    out[6]?.set(minX, maxY, maxZ);
    out[7]?.set(maxX, maxY, maxZ);
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
      nearDistance[0] = entryDistance;
    }
    if (exitDistance < farDistance[0]!) {
      farDistance[0] = exitDistance;
    }
    return nearDistance[0]! <= (farDistance[0]! * toleranceScale);
  }

  /**
  Intersects the ray with this bounding box directly (no separate ray-intersection
  box object needed), narrowing [tMin, tMax] to the overlap with the ray segment.
  */
  public intersectingSegment(ray: Ray | null, tMin: number[] | null, tMax: number[] | null): boolean {
    if (ray === null || tMin === null || tMin.length === 0 || tMax === null || tMax.length === 0) {
      return false;
    }

    const minimumDistance = tMin[0]!;
    const maximumDistance = tMax[0]!;
    const nearDistance = [minimumDistance];
    const farDistance = [maximumDistance];
    const toleranceScale = 1.0 + Numeric.EPSILON_FLOAT;

    if (!BoundingBox.clipAxisSlab(
      this.coordinates[BoundingBoxCoordinateIndex.MIN_X], this.coordinates[BoundingBoxCoordinateIndex.MAX_X],
      ray.position.x, ray.direction.x,
      toleranceScale,
      nearDistance, farDistance
    )) {
      return false;
    }
    if (!BoundingBox.clipAxisSlab(
      this.coordinates[BoundingBoxCoordinateIndex.MIN_Y], this.coordinates[BoundingBoxCoordinateIndex.MAX_Y],
      ray.position.y, ray.direction.y,
      toleranceScale,
      nearDistance, farDistance
    )) {
      return false;
    }
    if (!BoundingBox.clipAxisSlab(
      this.coordinates[BoundingBoxCoordinateIndex.MIN_Z], this.coordinates[BoundingBoxCoordinateIndex.MAX_Z],
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
