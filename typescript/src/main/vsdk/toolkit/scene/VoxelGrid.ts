import { Logger } from "../common/logging/Logger";
import { Numeric } from "../common/linealAlgebra/Numeric";
import { Ray } from "../common/linealAlgebra/Ray";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { BoundingBox } from "../skin/AxisAlignedBoundingBox";
import { Compound } from "../skin/Compound";
import { Geometry } from "../skin/Geometry";
import { MinMaxBox } from "../skin/MinMaxBox";
import { Patch } from "../environment/geometry/elements/Patch";
import { RayHit } from "../environment/geometry/elements/RayHit";
import { VoxelData } from "./VoxelData";
import { VoxelDataFlags } from "./VoxelDataFlags";

export class VoxelGrid {
  private static readonly MINIMUM_ELEMENT_COUNT_PER_CELL = 10;
  private static readonly DELTA_BOUND_FACTOR = 1e-4;

  private static subGridsToDelete: VoxelGrid[] | null = null;
  private static voxelCellsToDelete: VoxelData[] | null = null;
  private static constructorLevel = 0;
  private static randomRayCounter = 0;

  private xSize: number;
  private ySize: number;
  private zSize: number;
  private voxelSize: Vector3D;
  private volumeListsOfItems: Array<VoxelData[] | null> | null;
  private gridItemPool: unknown;
  private boundingBox: BoundingBox;
  private rayIntersectionBox: MinMaxBox | null;

  private static addToSubGridsDeletionCache(voxelGrid: VoxelGrid): void {
    if (VoxelGrid.subGridsToDelete === null) {
      VoxelGrid.subGridsToDelete = [];
    }
    VoxelGrid.subGridsToDelete.push(voxelGrid);
  }

  private static addToCellsDeletionCache(cell: VoxelData): void {
    if (VoxelGrid.voxelCellsToDelete === null) {
      VoxelGrid.voxelCellsToDelete = [];
    }
    VoxelGrid.voxelCellsToDelete.push(cell);
  }

  private static clampVoxel(v: number, max: number): number {
    if (v < 0) {
      return 0;
    }
    if (v >= max) {
      return max - 1;
    }
    return v;
  }

  private static shouldSubdivide(geometry: Geometry): boolean {
    return geometry.itemCount >= VoxelGrid.MINIMUM_ELEMENT_COUNT_PER_CELL;
  }

  private voxel2x(px: number): number {
    return px * this.voxelSize.x + this.boundingBox.minX();
  }

  private voxel2y(py: number): number {
    return py * this.voxelSize.y + this.boundingBox.minY();
  }

  private voxel2z(pz: number): number {
    return pz * this.voxelSize.z + this.boundingBox.minZ();
  }

  private x2voxel(px: number): number {
    return this.voxelSize.x < Numeric.EPSILON
      ? 0
      : globalThis.Math.trunc((px - this.boundingBox.minX()) / this.voxelSize.x);
  }

  private y2voxel(py: number): number {
    return this.voxelSize.y < Numeric.EPSILON
      ? 0
      : globalThis.Math.trunc((py - this.boundingBox.minY()) / this.voxelSize.y);
  }

  private z2voxel(pz: number): number {
    return this.voxelSize.z < Numeric.EPSILON
      ? 0
      : globalThis.Math.trunc((pz - this.boundingBox.minZ()) / this.voxelSize.z);
  }

  private cellIndexAddress(a: number, b: number, c: number): number {
    return (a * this.ySize + b) * this.zSize + c;
  }

  private putGeometryInsideVoxelGrid(geometry: Geometry, na: number, nb: number, nc: number): void {
    if (na <= 0 || nb <= 0 || nc <= 0) {
      Logger.error("VoxelGrid::putGeometryInsideVoxelGrid", "Invalid grid dimensions");
      process.exit(1);
    }

    this.boundingBox.copyFrom(geometry.boundingBox);
    this.boundingBox.enlargeByFactor(VoxelGrid.DELTA_BOUND_FACTOR);
    if (this.rayIntersectionBox === null) {
      this.rayIntersectionBox = new MinMaxBox(this.boundingBox);
    }
    else {
      this.rayIntersectionBox.updateFromBoundingBox(this.boundingBox);
    }
    this.xSize = na;
    this.ySize = nb;
    this.zSize = nc;

    this.voxelSize = this.boundingBox.voxelSize(na, nb, nc);
    this.volumeListsOfItems = new Array<VoxelData[] | null>(na * nb * nc).fill(null);
    this.gridItemPool = null;
    this.putSubGeometryInsideVoxelGrid(geometry);
  }

  private isSmall(bb: BoundingBox): boolean {
    return bb.dx() <= this.voxelSize.x &&
      bb.dy() <= this.voxelSize.y &&
      bb.dz() <= this.voxelSize.z;
  }

  private putSubGeometryInsideVoxelGrid(geometry: Geometry): void {
    if (this.isSmall(geometry.boundingBox)) {
      if (VoxelGrid.shouldSubdivide(geometry)) {
        this.insertSubGrid(geometry);
      }
      else {
        this.insertGeometryAsVoxelData(geometry);
      }
      return;
    }

    if (geometry.isCompound()) {
      this.processCompoundGeometry(geometry);
      return;
    }
    this.processPatches(geometry);
  }

  private putItemInsideVoxelGrid(item: VoxelData, itemBounds: BoundingBox): void {
    const boundaries = new BoundingBox();
    boundaries.copyFrom(itemBounds);
    boundaries.enlargeByFactor(VoxelGrid.DELTA_BOUND_FACTOR);

    const minVoxel = this.toVoxelClamped(boundaries.minPoint());
    const maxVoxel = this.toVoxelClamped(boundaries.maxPoint());

    const minA = minVoxel.x;
    const minB = minVoxel.y;
    const minC = minVoxel.z;
    const maxA = maxVoxel.x;
    const maxB = maxVoxel.y;
    const maxC = maxVoxel.z;

    for (let a = minA; a <= maxA; a++) {
      for (let b = minB; b <= maxB; b++) {
        for (let c = minC; c <= maxC; c++) {
          const index = this.cellIndexAddress(a, b, c);
          let voxelList = this.volumeListsOfItems![index];

          if (voxelList === null) {
            voxelList = [];
            this.volumeListsOfItems![index] = voxelList;
          }

          voxelList.push(item);
        }
      }
    }
  }

  private putPatchInsideVoxelGrid(patch: Patch): void {
    const localBounds = new BoundingBox();
    if (patch.boundingBox !== null) {
      localBounds.copyFrom(patch.boundingBox);
    }
    else {
      patch.computeAndGetBoundingBox(localBounds);
    }

    const voxelData = new VoxelData(patch, VoxelDataFlags.VOXEL_DATA_PATCH_MASK);
    this.putItemInsideVoxelGrid(voxelData, localBounds);
    VoxelGrid.addToCellsDeletionCache(voxelData);
  }

  private toVoxelClamped(p: Vector3D): Vector3D {
    return new Vector3D(
      VoxelGrid.clampVoxel(this.x2voxel(p.x), this.xSize),
      VoxelGrid.clampVoxel(this.y2voxel(p.y), this.ySize),
      VoxelGrid.clampVoxel(this.z2voxel(p.z), this.zSize)
    );
  }

  private insertGeometryAsVoxelData(geometry: Geometry): void {
    const voxelData = new VoxelData(geometry, VoxelDataFlags.VOXEL_DATA_GEOMETRY_MASK);
    this.putItemInsideVoxelGrid(voxelData, geometry.boundingBox);
    VoxelGrid.addToCellsDeletionCache(voxelData);
  }

  private processCompoundGeometry(geometry: Geometry): void {
    const geometryList = (geometry as Compound).children;
    for (let i = 0; geometryList !== null && i < geometryList.length; i++) {
      this.putSubGeometryInsideVoxelGrid(geometryList[i]);
    }
  }

  private processPatches(geometry: Geometry): void {
    const patches = Geometry.patchListReference(geometry);
    for (let i = 0; patches !== null && i < patches.length; i++) {
      this.putPatchInsideVoxelGrid(patches[i]);
    }
  }

  private insertSubGrid(geometry: Geometry): void {
    const subGrid = new VoxelGrid(geometry);
    const voxelData = new VoxelData(subGrid, VoxelDataFlags.VOXEL_DATA_GRID_MASK);

    this.putItemInsideVoxelGrid(voxelData, subGrid.boundingBox);

    VoxelGrid.addToSubGridsDeletionCache(subGrid);
    VoxelGrid.addToCellsDeletionCache(voxelData);
  }

  private gridBoundsIntersect(
    ray: Ray,
    minimumDistance: number,
    maximumDistance: number,
    t0: number[],
    position: Vector3D
  ): boolean {
    t0[0] = minimumDistance;
    position.sumScaled(ray.position, t0[0], ray.direction);
    if (this.boundingBox.outOfBounds(position)) {
      t0[0] = maximumDistance;
      if (this.rayIntersectionBox === null) {
        this.rayIntersectionBox = new MinMaxBox(this.boundingBox);
      }
      if (!this.rayIntersectionBox.intersect(ray, minimumDistance, t0)) {
        return false;
      }
      position.sumScaled(ray.position, t0[0], ray.direction);
    }

    return true;
  }

  private gridTraceSetup(
    ray: Ray,
    t0: number,
    p: Vector3D,
    g: number[],
    tDelta: Vector3D,
    tNext: Vector3D,
    step: number[],
    out: number[]
  ): void {
    g[0] = this.x2voxel(p.x);
    if (g[0] >= this.xSize) {
      g[0] = this.xSize - 1;
    }
    g[1] = this.y2voxel(p.y);
    if (g[1] >= this.ySize) {
      g[1] = this.ySize - 1;
    }
    g[2] = this.z2voxel(p.z);
    if (g[2] >= this.zSize) {
      g[2] = this.zSize - 1;
    }

    if (globalThis.Math.abs(ray.direction.x) > Numeric.EPSILON) {
      if (ray.direction.x > 0.0) {
        tDelta.x = this.voxelSize.x / ray.direction.x;
        tNext.x = t0 + (this.voxel2x(g[0] + 1) - p.x) / ray.direction.x;
        step[0] = 1;
        out[0] = this.xSize;
      }
      else {
        tDelta.x = this.voxelSize.x / -ray.direction.x;
        tNext.x = t0 + (this.voxel2x(g[0]) - p.x) / ray.direction.x;
        step[0] = -1;
        out[0] = -1;
      }
    }
    else {
      tDelta.x = 0.0;
      tNext.x = Numeric.HUGE_FLOAT_VALUE;
    }

    if (globalThis.Math.abs(ray.direction.y) > Numeric.EPSILON) {
      if (ray.direction.y > 0.0) {
        tDelta.y = this.voxelSize.y / ray.direction.y;
        tNext.y = t0 + (this.voxel2y(g[1] + 1) - p.y) / ray.direction.y;
        step[1] = 1;
        out[1] = this.ySize;
      }
      else {
        tDelta.y = this.voxelSize.y / -ray.direction.y;
        tNext.y = t0 + (this.voxel2y(g[1]) - p.y) / ray.direction.y;
        step[1] = -1;
        out[1] = -1;
      }
    }
    else {
      tDelta.y = 0.0;
      tNext.y = Numeric.HUGE_FLOAT_VALUE;
    }

    if (globalThis.Math.abs(ray.direction.z) > Numeric.EPSILON) {
      if (ray.direction.z > 0.0) {
        tDelta.z = this.voxelSize.z / ray.direction.z;
        tNext.z = t0 + (this.voxel2z(g[2] + 1) - p.z) / ray.direction.z;
        step[2] = 1;
        out[2] = this.zSize;
      }
      else {
        tDelta.z = this.voxelSize.z / -ray.direction.z;
        tNext.z = t0 + (this.voxel2z(g[2]) - p.z) / ray.direction.z;
        step[2] = -1;
        out[2] = -1;
      }
    }
    else {
      tDelta.z = 0.0;
      tNext.z = Numeric.HUGE_FLOAT_VALUE;
    }
  }

  private static nextVoxel(
    t0: number[],
    g: number[],
    tNext: Vector3D,
    tDelta: Vector3D,
    step: number[],
    out: number[]
  ): boolean {
    let inGrid: number;

    if (tNext.x <= tNext.y && tNext.x <= tNext.z) {
      g[0] += step[0];
      t0[0] = tNext.x;
      tNext.x += tDelta.x;
      inGrid = g[0] - out[0];
    }
    else if (tNext.y <= tNext.z) {
      g[1] += step[1];
      t0[0] = tNext.y;
      tNext.y += tDelta.y;
      inGrid = g[1] - out[1];
    }
    else {
      g[2] += step[2];
      t0[0] = tNext.z;
      tNext.z += tDelta.z;
      inGrid = g[2] - out[2];
    }
    return inGrid !== 0;
  }

  private static voxelIntersect(
    items: VoxelData[] | null,
    ray: Ray,
    counter: number,
    minimumDistance: number,
    maximumDistance: number[],
    hitFlags: number,
    hitStore: RayHit | null
  ): RayHit | null {
    let hit: RayHit | null = null;

    for (let i = 0; items !== null && i < items.length; i++) {
      const item = items[i];
      if (item.lastRayId() !== counter) {
        let h: RayHit | null = null;
        if (item.isPatch() && item.patch !== null) {
          h = item.patch.intersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
        }
        else if (item.isGeom() && item.geometry !== null) {
          h = item.geometry.discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
        }
        else if (item.isGrid() && item.voxelGrid !== null) {
          h = item.voxelGrid.gridIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
        }
        if (h !== null) {
          hit = h;
        }

        item.updateRayId(counter);
      }
    }

    return hit;
  }

  private static randomRayId(): number {
    VoxelGrid.randomRayCounter++;
    return VoxelGrid.randomRayCounter & VoxelDataFlags.VOXEL_DATA_RAY_COUNT_MASK;
  }

  public gridIntersect(
    ray: Ray,
    minimumDistance: number,
    maximumDistance: number[],
    hitFlags: number,
    hitStore: RayHit | null
  ): RayHit | null {
    const tNext = new Vector3D();
    const tDelta = new Vector3D();
    const p = new Vector3D();
    const step = [0, 0, 0];
    const out = [0, 0, 0];
    const g = [0, 0, 0];
    let hit: RayHit | null = null;
    const t0 = [0.0];

    if (!this.gridBoundsIntersect(ray, minimumDistance, maximumDistance[0], t0, p)) {
      return null;
    }

    this.gridTraceSetup(ray, t0[0], p, g, tDelta, tNext, step, out);

    const counter = VoxelGrid.randomRayId();

    do {
      const list = this.volumeListsOfItems![this.cellIndexAddress(g[0], g[1], g[2])];
      if (list !== null) {
        const h = VoxelGrid.voxelIntersect(list, ray, counter, t0[0], maximumDistance, hitFlags, hitStore);
        if (h !== null) {
          hit = h;
        }
      }
    } while (VoxelGrid.nextVoxel(t0, g, tNext, tDelta, step, out) && t0[0] <= maximumDistance[0]);

    return hit;
  }

  public constructor(geometry: Geometry) {
    this.boundingBox = new BoundingBox();
    this.rayIntersectionBox = null;
    this.xSize = 0;
    this.ySize = 0;
    this.zSize = 0;
    this.voxelSize = new Vector3D();
    this.volumeListsOfItems = null;
    this.gridItemPool = null;

    const p = globalThis.Math.pow(geometry.itemCount, 0.33333) + 1;
    const gridSize = globalThis.Math.floor(p);
    process.stderr.write(
      `Setting ${geometry.itemCount} volumeListsOfItems in ${gridSize}^3 cells level ${VoxelGrid.constructorLevel} voxel grid ... \n`
    );
    VoxelGrid.constructorLevel++;

    this.putGeometryInsideVoxelGrid(geometry, gridSize, gridSize, gridSize);

    VoxelGrid.constructorLevel--;
  }

  public static freeVoxelGridElements(): void {
    if (VoxelGrid.voxelCellsToDelete !== null) {
      VoxelGrid.voxelCellsToDelete.length = 0;
      VoxelGrid.voxelCellsToDelete = null;
    }

    if (VoxelGrid.subGridsToDelete !== null) {
      for (let i = 0; i < VoxelGrid.subGridsToDelete.length; i++) {
        const subGrid = VoxelGrid.subGridsToDelete[i];
        subGrid.gridItemPool = null;
        subGrid.volumeListsOfItems = null;
      }
      VoxelGrid.subGridsToDelete.length = 0;
      VoxelGrid.subGridsToDelete = null;
    }
  }

  public print(): void {
    process.stdout.write(`DX: ${globalThis.Math.trunc(this.xSize)}, DY: ${globalThis.Math.trunc(this.ySize)}, DZ: ${globalThis.Math.trunc(this.zSize)}\n`);

    for (let z = 0; z < this.zSize; z++) {
      process.stdout.write(`Z level ${z + 1} of ${this.zSize}\n`);

      for (let y = 0; y < this.ySize; y++) {
        process.stdout.write("  | ");
        for (let x = 0; x < this.xSize; x++) {
          const list = this.volumeListsOfItems![this.cellIndexAddress(z, y, x)];
          if (list === null) {
            process.stdout.write("[  ]");
          }
          else {
            process.stdout.write(`(${list.length.toString().padStart(2, " ")})`);
          }
          process.stdout.write(" ");
        }
        process.stdout.write(" |\n");
      }
    }
  }
}
