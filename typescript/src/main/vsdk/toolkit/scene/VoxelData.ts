import { Geometry } from "../skin/Geometry";
import { Patch } from "../skin/Patch";
import { VoxelDataFlags } from "./VoxelDataFlags";
import type { VoxelGrid } from "./VoxelGrid";

export class VoxelData {
  public patch: Patch | null;
  public geometry: Geometry | null;
  public voxelGrid: VoxelGrid | null;
  public flags: number;

  public constructor(data: VoxelGrid, flags: number);
  public constructor(data: Patch, flags: number);
  public constructor(data: Geometry, flags: number);
  public constructor(data: VoxelGrid | Patch | Geometry, flags: number) {
    this.geometry = null;
    this.patch = null;
    this.voxelGrid = null;
    this.flags = flags;

    if ("gridIntersect" in (data as object)) {
      this.voxelGrid = data as VoxelGrid;
    }
    else if ("numberOfVertices" in (data as object)) {
      this.patch = data as Patch;
    }
    else {
      this.geometry = data as Geometry;
    }
  }

  public updateRayId(id: number): void {
    this.flags = (this.flags & ~VoxelDataFlags.VOXEL_DATA_RAY_COUNT_MASK) | (id & VoxelDataFlags.VOXEL_DATA_RAY_COUNT_MASK);
  }

  public lastRayId(): number {
    return this.flags & VoxelDataFlags.VOXEL_DATA_RAY_COUNT_MASK;
  }

  public isPatch(): boolean {
    return (this.flags & VoxelDataFlags.VOXEL_DATA_PATCH_MASK) !== 0;
  }

  public isGeom(): boolean {
    return (this.flags & VoxelDataFlags.VOXEL_DATA_GEOMETRY_MASK) !== 0;
  }

  public isGrid(): boolean {
    return (this.flags & VoxelDataFlags.VOXEL_DATA_GRID_MASK) !== 0;
  }
}
