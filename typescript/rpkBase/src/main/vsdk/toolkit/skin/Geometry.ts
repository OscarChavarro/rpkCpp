import { Logger as VsdkLogger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { Ray } from "vitral/dist/vsdk/toolkit/environment/geometry/element/Ray";
import { Statistics } from "../common/statistics/Statistics";
import { RayHitFlag } from "../environment/geometry/elements/RayHitFlag";
import { BoundingBox } from "./AxisAlignedBoundingBox";
import { GeometryClassId } from "./GeometryClassId";
import type { Element } from "../environment/geometry/elements/Element";
import type { Patch } from "../environment/geometry/elements/Patch";
import type { RayExclusionSet } from "../environment/geometry/elements/RayExclusionSet";
import type { RayHit } from "../environment/geometry/elements/RayHit";
import type { PatchSet } from "../environment/geometry/elements/PatchSet";
import type { MeshSurface } from "./MeshSurface";
import type { Compound } from "./Compound";

export class Geometry {
  public static nextGeometryId = 0;

  // Interval [firstFaceIndex, lastFaceIndex] of Patch.getFaceIndex() values
  // of the patches below this geometry. Scene.buildFaceTable() numbers the
  // faces depth-first over the cluster hierarchy, so every geometry of that
  // hierarchy covers a contiguous interval. Empty (first > last) until the
  // face table is built.
  private firstFaceIndex: number;
  private lastFaceIndex: number;

  public id: number;
  public boundingBox: BoundingBox;
  public radianceData: Element | null;
  public itemCount: number;
  public bounded: boolean;
  public shaftCullGeometry: boolean;
  public isDuplicate: boolean;
  public className: number;

  public constructor();
  public constructor(inClassName: number);
  public constructor(inClassName?: number) {
    this.firstFaceIndex = 0;
    this.lastFaceIndex = -1;
    if (inClassName === undefined) {
      this.id = 0;
      this.boundingBox = new BoundingBox();
      this.radianceData = null;
      this.itemCount = 0;
      this.bounded = false;
      this.isDuplicate = false;
      this.className = GeometryClassId.UNDEFINED;
      this.shaftCullGeometry = false;
      return;
    }

    Statistics.instance().reader.numberOfGeometries++;
    this.id = Geometry.nextGeometryId;
    Geometry.nextGeometryId++;
    this.className = inClassName;
    this.isDuplicate = false;
    this.bounded = false;
    this.shaftCullGeometry = false;
    this.radianceData = null;
    this.itemCount = 0;
    this.boundingBox = new BoundingBox();
  }

  public destroy(): void {
    if (this.radianceData !== null && !this.isDuplicate) {
      this.radianceData = null;
    }
  }

  public getBoundingBox(): BoundingBox {
    return this.boundingBox;
  }

  public static destroy(geometry: Geometry | null): void {
    if (geometry === null) {
      return;
    }
    geometry.destroy();
    Statistics.instance().reader.numberOfGeometries--;
  }

  public isCompound(): boolean {
    return this.className === GeometryClassId.COMPOUND;
  }

  public getFirstFaceIndex(): number {
    return this.firstFaceIndex;
  }

  public getLastFaceIndex(): number {
    return this.lastFaceIndex;
  }

  /**
  Records the interval of face indices covered by this geometry. Called by
  Scene.buildFaceTable() only; an empty interval is first > last
  */
  public setFaceIndexRange(first: number, last: number): void {
    this.firstFaceIndex = first;
    this.lastFaceIndex = last;
  }

  private static cloneGeometryList(input: Geometry[] | null): Geometry[] {
    const output: Geometry[] = [];
    for (let i = 0; input !== null && i < input.length; i++) {
      output.push(input[i]!);
    }
    return output;
  }

  public static primitiveListCopy(geometry: Geometry): Geometry[] | null {
    if (geometry.isCompound()) {
      const comp = geometry as unknown as Compound;
      return Geometry.cloneGeometryList(comp.children);
    }
    return null;
  }

  public static patchListReference(geometry: Geometry): Patch[] | null {
    if (geometry.className === GeometryClassId.SURFACE_MESH) {
      return ((geometry as unknown) as MeshSurface).faces;
    }
    if (geometry.className === GeometryClassId.PATCH_SET) {
      return ((geometry as unknown) as PatchSet).getPatchList();
    }
    return null;
  }

  public clone(): Geometry {
    if (this.className !== GeometryClassId.PATCH_SET) {
      VsdkLogger.reportMessage(this, VsdkLogger.FATAL_ERROR, "duplicateIfPatchSet", "this should not happen");
    }

    const PatchSetClass = require("../environment/geometry/elements/PatchSet").PatchSet as { new(input: Patch[]): PatchSet };
    const newPatchSet = new PatchSetClass(Geometry.patchListReference(this) ?? []);

    newPatchSet.id = Statistics.instance().reader.numberOfGeometries;
    newPatchSet.boundingBox = this.boundingBox;
    newPatchSet.radianceData = this.radianceData;
    newPatchSet.itemCount = this.itemCount;
    newPatchSet.bounded = this.bounded;
    newPatchSet.shaftCullGeometry = this.shaftCullGeometry;
    newPatchSet.className = this.className;
    newPatchSet.isDuplicate = true;
    newPatchSet.setFaceIndexRange(this.firstFaceIndex, this.lastFaceIndex);

    Statistics.instance().reader.numberOfGeometries++;
    return newPatchSet as unknown as Geometry;
  }

  // Scratch storage reused across pre-test calls (this method runs billions
  // of times per scene on the single threaded rendering core; the C++ port
  // keeps these on the stack)
  private static readonly preTestScratchDistance = [0.0];

  public discretizationIntersectPreTest(ray: Ray, minimumDistance: number, maximumDistance: number[], exclusions: RayExclusionSet): boolean {
    if (exclusions.excludesGeometry(this)) {
      return false;
    }

    if (this.bounded) {
      const vTmp = ray.getOrigin().add(ray.getDirection().multiply(minimumDistance));
      if (this.boundingBox.outOfBounds(vTmp)) {
        const nMaximumDistance = Geometry.preTestScratchDistance;
        nMaximumDistance[0] = maximumDistance[0]!;
        if (!this.boundingBox.intersect(ray, minimumDistance, nMaximumDistance)) {
          return false;
        }
      }
    }

    return true;
  }

  public discretizationIntersect(
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

    if (this.className === GeometryClassId.SURFACE_MESH) {
      return ((this as unknown) as MeshSurface).discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
    }
    if (this.className === GeometryClassId.COMPOUND) {
      return ((this as unknown) as Compound).discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
    }
    if (this.className === GeometryClassId.PATCH_SET) {
      return ((this as unknown) as PatchSet).discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
    }
    return null;
  }

  public static listDiscretizationIntersect(
    geometryList: Geometry[] | null,
    ray: Ray,
    minimumDistance: number,
    maximumDistance: number[],
    hitFlags: number,
    hitStore: RayHit | null,
    exclusions: RayExclusionSet
  ): RayHit | null {
    let hit: RayHit | null = null;

    for (let i = 0; geometryList !== null && i < geometryList.length; i++) {
      const h = geometryList[i]!.discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
      if (h !== null) {
        if ((hitFlags & RayHitFlag.ANY) !== 0) {
          return h;
        }
        hit = h;
      }
    }
    return hit;
  }

  public static listBounds(geometryList: Geometry[] | null, boundingBox: BoundingBox): void {
    for (let i = 0; geometryList !== null && i < geometryList.length; i++) {
      boundingBox.enlarge(geometryList[i]!.boundingBox);
    }
  }

  public static patchListIntersect(
    patchList: Patch[] | null,
    ray: Ray,
    minimumDistance: number,
    maximumDistance: number[],
    hitFlags: number,
    hitStore: RayHit | null,
    exclusions: RayExclusionSet
  ): RayHit | null {
    let hit: RayHit | null = null;
    for (let i = 0; patchList !== null && i < patchList.length; i++) {
      const h = patchList[i]!.intersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
      if (h !== null) {
        if ((hitFlags & RayHitFlag.ANY) !== 0) {
          return h;
        }
        hit = h;
      }
    }
    return hit;
  }

  protected static patchListBounds(patchList: Patch[] | null, boundingBox: BoundingBox): BoundingBox {
    const currentPatchBoundingBox = new BoundingBox();

    for (let i = 0; patchList !== null && i < patchList.length; i++) {
      patchList[i]!.computeBounds(currentPatchBoundingBox);
      boundingBox.enlarge(currentPatchBoundingBox);
    }

    return boundingBox;
  }
}
