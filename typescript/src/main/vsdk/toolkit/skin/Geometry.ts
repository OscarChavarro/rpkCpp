import { Error as VsdkError } from "../common/Error";
import { Ray } from "../common/linealAlgebra/Ray";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { Statistics } from "../common/statistics/Statistics";
import { RayHitFlag } from "../skin/RayHitFlag";
import { BoundingBox } from "./BoundingBox";
import { GeometryClassId } from "./GeometryClassId";
import { MinMaxBox } from "./MinMaxBox";
import type { Element } from "./Element";
import type { Patch } from "./Patch";
import type { RayHit } from "./RayHit";
import type { PatchSet } from "./PatchSet";
import type { MeshSurface } from "./MeshSurface";
import type { Compound } from "./Compound";

export class Geometry {
  public static nextGeometryId = 0;
  public static excludedGeometry1: Geometry | null = null;
  public static excludedGeometry2: Geometry | null = null;

  public id: number;
  public boundingBox: BoundingBox;
  public rayIntersectionBox: MinMaxBox | null;
  public radianceData: Element | null;
  public itemCount: number;
  public bounded: boolean;
  public shaftCullGeometry: boolean;
  public omit: boolean;
  public isDuplicate: boolean;
  public className: number;

  public constructor();
  public constructor(inClassName: number);
  public constructor(inClassName?: number) {
    if (inClassName === undefined) {
      this.id = 0;
      this.boundingBox = new BoundingBox();
      this.rayIntersectionBox = null;
      this.radianceData = null;
      this.itemCount = 0;
      this.bounded = false;
      this.omit = false;
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
    this.rayIntersectionBox = null;
    this.radianceData = null;
    this.itemCount = 0;
    this.omit = false;
    this.boundingBox = new BoundingBox();
  }

  public destroy(): void {
    this.rayIntersectionBox = null;
    if (this.radianceData !== null && !this.isDuplicate) {
      this.radianceData = null;
    }
  }

  public getBoundingBox(): BoundingBox {
    return this.boundingBox;
  }

  public getRayIntersectionBox(): MinMaxBox {
    if (this.rayIntersectionBox === null) {
      this.rayIntersectionBox = new MinMaxBox(this.boundingBox);
    }
    else {
      this.rayIntersectionBox.updateFromBoundingBox(this.boundingBox);
    }
    return this.rayIntersectionBox;
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

  private static cloneGeometryList(input: Geometry[] | null): Geometry[] {
    const output: Geometry[] = [];
    for (let i = 0; input !== null && i < input.length; i++) {
      output.push(input[i]);
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
      VsdkError.fatal(666, "duplicateIfPatchSet", "this should not happen");
    }

    const PatchSetClass = require("./PatchSet").PatchSet as { new(input: Patch[]): PatchSet };
    const newPatchSet = new PatchSetClass(Geometry.patchListReference(this) ?? []);

    newPatchSet.id = Statistics.instance().reader.numberOfGeometries;
    newPatchSet.boundingBox = this.boundingBox;
    newPatchSet.radianceData = this.radianceData;
    newPatchSet.itemCount = this.itemCount;
    newPatchSet.bounded = this.bounded;
    newPatchSet.shaftCullGeometry = this.shaftCullGeometry;
    newPatchSet.omit = this.omit;
    newPatchSet.className = this.className;
    newPatchSet.isDuplicate = true;

    Statistics.instance().reader.numberOfGeometries++;
    return newPatchSet as unknown as Geometry;
  }

  public static dontIntersect(geometry1: Geometry | null, geometry2: Geometry | null): void {
    Geometry.excludedGeometry1 = geometry1;
    Geometry.excludedGeometry2 = geometry2;
  }

  public discretizationIntersectPreTest(ray: Ray, minimumDistance: number, maximumDistance: number[]): boolean {
    if (this === Geometry.excludedGeometry1 || this === Geometry.excludedGeometry2) {
      return false;
    }

    if (this.bounded) {
      const vTmp = new Vector3D();
      vTmp.sumScaled(ray.position, minimumDistance, ray.direction);
      if (this.boundingBox.outOfBounds(vTmp)) {
        const nMaximumDistance = [maximumDistance[0]];
        const minMaxBox = this.getRayIntersectionBox();
        if (!minMaxBox.intersect(ray, minimumDistance, nMaximumDistance)) {
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
    hitStore: RayHit | null
  ): RayHit | null {
    if (!this.discretizationIntersectPreTest(ray, minimumDistance, maximumDistance)) {
      return null;
    }

    if (this.className === GeometryClassId.SURFACE_MESH) {
      return ((this as unknown) as MeshSurface).discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    }
    if (this.className === GeometryClassId.COMPOUND) {
      return ((this as unknown) as Compound).discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    }
    if (this.className === GeometryClassId.PATCH_SET) {
      return ((this as unknown) as PatchSet).discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    }
    return null;
  }

  public static listDiscretizationIntersect(
    geometryList: Geometry[] | null,
    ray: Ray,
    minimumDistance: number,
    maximumDistance: number[],
    hitFlags: number,
    hitStore: RayHit | null
  ): RayHit | null {
    let hit: RayHit | null = null;

    for (let i = 0; geometryList !== null && i < geometryList.length; i++) {
      const h = geometryList[i].discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
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
      boundingBox.enlarge(geometryList[i].boundingBox);
    }
  }

  public static patchListIntersect(
    patchList: Patch[] | null,
    ray: Ray,
    minimumDistance: number,
    maximumDistance: number[],
    hitFlags: number,
    hitStore: RayHit | null
  ): RayHit | null {
    let hit: RayHit | null = null;
    for (let i = 0; patchList !== null && i < patchList.length; i++) {
      const h = patchList[i].intersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
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
      patchList[i].computeAndGetBoundingBox(currentPatchBoundingBox);
      boundingBox.enlarge(currentPatchBoundingBox);
    }

    return boundingBox;
  }

  public isExcluded(): boolean {
    return this === Geometry.excludedGeometry1 || this === Geometry.excludedGeometry2;
  }
}
