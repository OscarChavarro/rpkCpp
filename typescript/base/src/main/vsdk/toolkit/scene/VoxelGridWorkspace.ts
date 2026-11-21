import { Vector3D } from "../common/linealAlgebra/Vector3D";
import type { VisibilityQueryRecorder } from "./VisibilityQueryRecorder";
import { VoxelDataFlags } from "./VoxelDataFlags";

/**
Traversal variables of one gridIntersect call at a given nesting depth
*/
export class VoxelGridTraversalFrame {
  public readonly tNext = new Vector3D();
  public readonly tDelta = new Vector3D();
  public readonly p = new Vector3D();
  public readonly step = [0, 0, 0];
  public readonly out = [0, 0, 0];
  public readonly g = [0, 0, 0];
  public readonly t0 = [0.0];
}

/**
Per-query scratch of a VoxelGrid traversal: the ray counter and, for every
item of the grid hierarchy, the counter of the last ray that tested it. Items
spanning several cells are so tested only once per ray.

Workspace: mutable, owned by one caller and passed explicitly on every
gridIntersect call. The grid itself is never written during a query.

When ray capture is enabled (RPK_RAY_CAPTURE), the callers of the visibility
queries report each query to the recorder the workspace carries (null
otherwise).

TypeScript adaptation: the traversal variables that the C++ port keeps on the
stack live here too, one frame per grid nesting depth, so a query allocates
nothing.
*/
export class VoxelGridWorkspace {
  private readonly lastRayIdByItem: Int32Array;
  private readonly frames: VoxelGridTraversalFrame[];
  private rayCount: number;
  private readonly recorder: VisibilityQueryRecorder | null;

  public constructor(numberOfItems: number, numberOfLevels: number, inRecorder: VisibilityQueryRecorder | null = null) {
    this.recorder = inRecorder;
    this.lastRayIdByItem = new Int32Array(globalThis.Math.max(numberOfItems, 1));
    this.frames = [];
    for (let i = 0; i < globalThis.Math.max(numberOfLevels, 1); i++) {
      this.frames.push(new VoxelGridTraversalFrame());
    }
    this.rayCount = 0;
  }

  public nextRayId(): number {
    this.rayCount = (this.rayCount + 1) | 0;
    return this.rayCount & VoxelDataFlags.VOXEL_DATA_RAY_COUNT_MASK;
  }

  public lastRayId(itemIndex: number): number {
    return this.lastRayIdByItem[itemIndex]!;
  }

  public updateRayId(itemIndex: number, id: number): void {
    this.lastRayIdByItem[itemIndex] = id & VoxelDataFlags.VOXEL_DATA_RAY_COUNT_MASK;
  }

  public frame(depth: number): VoxelGridTraversalFrame {
    return this.frames[depth]!;
  }

  public getNumberOfItems(): number {
    return this.lastRayIdByItem.length;
  }

  public getRecorder(): VisibilityQueryRecorder | null {
    return this.recorder;
  }
}
