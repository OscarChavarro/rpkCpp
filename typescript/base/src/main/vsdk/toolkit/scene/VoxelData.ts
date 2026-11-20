import { Geometry } from "../skin/Geometry";
import { Patch } from "../environment/geometry/elements/Patch";
import { VoxelDataFlags } from "./VoxelDataFlags";
import type { VoxelGrid } from "./VoxelGrid";

/**
One item stored in the cells of a VoxelGrid: a patch, a geometry or a sub
grid. The index is dense over the whole grid hierarchy and addresses the
per-item scratch of a VoxelGridWorkspace.

Shared, read-only after the grid is built.
*/
export class VoxelData {
  private readonly patch: Patch | null;
  private readonly geometry: Geometry | null;
  private readonly voxelGrid: VoxelGrid | null;
  private readonly flags: number; // Patch, geometry or grid
  private readonly index: number;

  public constructor(data: VoxelGrid, flags: number, index: number);
  public constructor(data: Patch, flags: number, index: number);
  public constructor(data: Geometry, flags: number, index: number);
  public constructor(data: VoxelGrid | Patch | Geometry, flags: number, index: number) {
    this.flags = flags;
    this.index = index;

    if ((flags & VoxelDataFlags.VOXEL_DATA_GRID_MASK) !== 0) {
      this.voxelGrid = data as VoxelGrid;
      this.patch = null;
      this.geometry = null;
    }
    else if ((flags & VoxelDataFlags.VOXEL_DATA_PATCH_MASK) !== 0) {
      this.voxelGrid = null;
      this.patch = data as Patch;
      this.geometry = null;
    }
    else {
      this.voxelGrid = null;
      this.patch = null;
      this.geometry = data as Geometry;
    }
  }

  public getPatch(): Patch | null {
    return this.patch;
  }

  public getGeometry(): Geometry | null {
    return this.geometry;
  }

  public getVoxelGrid(): VoxelGrid | null {
    return this.voxelGrid;
  }

  public getIndex(): number {
    return this.index;
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
