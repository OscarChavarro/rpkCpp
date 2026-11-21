//= References:                                                             =
//= [SNYD1987] Snyder, John M. and Barr, Alan H. "Ray Tracing Complex       =
//=     Models Containing Surface Tessellations". SIGGRAPH '87, Computer    =
//=     Graphics 21(4), 1987, pp. 119-128.                                  =

import type { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd";
import type { Ray } from "../element/Ray";
import type { PrimitiveRayTest } from "./PrimitiveRayTest";
import { SpatialGroupTree } from "./SpatialGroupTree";
import { SpatialIndex } from "./SpatialIndex";
import type { SpatialIndexWorkspace } from "./SpatialIndexWorkspace";
import { SpatialQueryMode } from "./SpatialQueryMode";
import { SpatialQueryResult } from "./SpatialQueryResult";

// Smallest cell size and direction component the traversal divides by, and
// the smallest enlargement of a box side (RenderPark's EPSILON)
const EPSILON = 1e-6;

// Relative tolerance of the far end in a box/segment clip
const TOLERANCE_SCALE = 1.0 + 1e-6;

// Parameter of the next cell boundary along an axis the ray does not cross
const NO_BOUNDARY = 1e300;

function component(v: Vector3Dd, k: number): number {
  return k === 0 ? v.x() : (k === 1 ? v.y() : v.z());
}

function outOfBox(boxes: number[], offset: number, p: number[]): boolean {
  for (let k = 0; k < 3; k++) {
    if (p[k]! < boxes[offset + k]! || p[k]! > boxes[offset + 3 + k]!) {
      return true;
    }
  }
  return false;
}

/**
`SpatialIndex` over a hierarchy of uniform grids ([SNYD1987]), built from a
`SpatialGroupTree` as RenderPark builds its voxel grid from the scene
geometry:
- a grid over a group has floor(count^(1/3) + 1) cells along each axis,
  count being the number of primitives below the group, and covers the box
  of the group enlarged by DELTA_BOUND_FACTOR of its size;
- a member group whose box fits in one cell becomes a single item of the
  grid: a sub grid over it when it holds MINIMUM_ITEMS_FOR_SUB_GRID
  primitives or more, otherwise the group itself, whose members are tested
  in order behind a test of its box;
- a member group that does not fit in a cell is replaced by its members,
  and a primitive member is an item of its own;
- every item is listed in each cell its box, enlarged as the grid box, overlaps.

A query walks the cells the ray crosses in order and tests the items of
each cell in list order, each item once per query (the workspace mailbox, of
`getWorkspaceSize()` entries, one per item). The traversal ends after the
first cell entered beyond the parameter of the accepted hit.

ANY_HIT follows RenderPark's grid too: a group item ends at its first
accepted hit, and the traversal goes on, with the interval shortened to the
accepted hit, until it leaves the cells before it; the blocker returned is
the one RenderPark's grid returns, not necessarily the first accepted one.

Traversal arithmetic is in double precision and prunes only; the test of the
caller decides every hit and receives the query's minimum parameter.

Shared, read-only after construction: the index keeps its own copy of the
tree data it needs, and a query writes only to the workspace and the test
it is given.
*/
export class GridSpatialIndex extends SpatialIndex {
  public static readonly MINIMUM_ITEMS_FOR_SUB_GRID = 10;
  public static readonly DELTA_BOUND_FACTOR = 1e-4;

  private static readonly PRIMITIVE_ITEM = 0;
  private static readonly GROUP_ITEM = 1;
  private static readonly GRID_ITEM = 2;

  private readonly numberOfPrimitives: number;

  // Items, dense ids: kind and the primitive, group or grid they refer to
  private readonly itemKind: number[];
  private readonly itemReference: number[];

  // Grids: padded box (6 values), cells per axis, cell size (3 values),
  // first entry of the grid in cellStart (cells + 1 entries per grid)
  private readonly gridBoxes: number[];
  private readonly gridResolution: number[];
  private readonly gridCellSizes: number[];
  private readonly gridFirstCell: number[];
  private readonly cellStart: number[];
  private readonly cellItems: number[];

  // Groups used as items: padded box (6 values), bounded flag, members
  // (as in SpatialGroupTree.getMember) from groupFirstMember
  private readonly groupBoxes: number[];
  private readonly groupBounded: boolean[];
  private readonly groupFirstMember: number[];
  private readonly groupMembers: number[];

  /**
  @param tree the groups and the primitive boxes, unpadded (the index
  pads them by DELTA_BOUND_FACTOR); its root group must have a box, and
  only the primitives that are members of a group are indexed
  */
  public constructor(tree: SpatialGroupTree) {
    super();
    this.numberOfPrimitives = tree.getNumberOfPrimitives();
    this.itemKind = [];
    this.itemReference = [];
    this.gridBoxes = [];
    this.gridResolution = [];
    this.gridCellSizes = [];
    this.gridFirstCell = [];
    this.cellStart = [];
    this.cellItems = [];
    this.groupBoxes = [];
    this.groupBounded = [];
    this.groupFirstMember = [];
    this.groupMembers = [];

    const box = [0, 0, 0, 0, 0, 0];
    const padded = [0, 0, 0, 0, 0, 0];
    for (let g = 0; g < tree.getNumberOfGroups(); g++) {
      for (let k = 0; k < 6; k++) {
        box[k] = tree.getGroupBox(g, k);
      }
      GridSpatialIndex.enlargedBox(box, padded);
      for (let k = 0; k < 6; k++) {
        this.groupBoxes.push(padded[k]!);
      }
      this.groupBounded.push(tree.isBounded(g));
      this.groupFirstMember.push(this.groupMembers.length);
      for (let i = 0; i < tree.getNumberOfMembers(g); i++) {
        this.groupMembers.push(tree.getMember(g, i));
      }
    }
    this.groupFirstMember.push(this.groupMembers.length);

    this.buildGrid(tree, 0);
  }

  /**
  The box enlarged on each side by DELTA_BOUND_FACTOR of its size along that
  axis, and by at least EPSILON
  */
  private static enlargedBox(box: number[], out: number[]): void {
    for (let k = 0; k < 3; k++) {
      let delta = (box[3 + k]! - box[k]!) * GridSpatialIndex.DELTA_BOUND_FACTOR;
      if (delta < EPSILON) {
        delta = EPSILON;
      }
      out[k] = box[k]! - delta;
      out[3 + k] = box[3 + k]! + delta;
    }
  }

  /** Index of the cell holding coordinate p along one axis, clamped to the grid */
  private static cellCoordinate(p: number, lo: number, size: number, resolution: number): number {
    let v = size < EPSILON ? 0 : globalThis.Math.trunc((p - lo) / size);
    if (v < 0) {
      v = 0;
    }
    if (v >= resolution) {
      v = resolution - 1;
    }
    return v;
  }

  private newItem(kind: number, reference: number): number {
    this.itemKind.push(kind);
    this.itemReference.push(reference);
    return this.itemKind.length - 1;
  }

  private static primitiveBox(tree: SpatialGroupTree, member: number): number[] {
    const box = [0, 0, 0, 0, 0, 0];
    const primitiveBoxes = tree.getPrimitiveBoxes();
    for (let k = 0; k < 6; k++) {
      box[k] = primitiveBoxes[6 * member + k]!;
    }
    return box;
  }

  /**
  Builds the grid over a group (and, recursively, its sub grids) and returns
  its id
  */
  private buildGrid(tree: SpatialGroupTree, group: number): number {
    const grid = this.gridResolution.length;
    const count = tree.getPrimitiveCount(group);
    const resolution = globalThis.Math.trunc(globalThis.Math.floor(globalThis.Math.pow(count, 0.33333) + 1.0));

    for (let k = 0; k < 6; k++) {
      this.gridBoxes.push(this.groupBoxes[6 * group + k]!);
    }
    this.gridResolution.push(resolution);
    for (let k = 0; k < 3; k++) {
      this.gridCellSizes.push((this.gridBoxes[6 * grid + 3 + k]! - this.gridBoxes[6 * grid + k]!) / resolution);
    }
    this.gridFirstCell.push(0);

    const numberOfCells = resolution * resolution * resolution;
    const cells: Array<number[] | null> = new Array<number[] | null>(numberOfCells).fill(null);

    if (resolution === 1) {
      this.insert(tree, grid, cells, group);
    }
    else {
      // The group of a grid never fits in one of its cells (unless its box
      // is degenerate, where this avoids an endless recursion)
      for (let i = 0; i < tree.getNumberOfMembers(group); i++) {
        const member = tree.getMember(group, i);
        if (SpatialGroupTree.isGroupMember(member)) {
          this.insert(tree, grid, cells, SpatialGroupTree.memberGroup(member));
        }
        else {
          this.insertItem(grid, cells, this.newItem(GridSpatialIndex.PRIMITIVE_ITEM, member), GridSpatialIndex.primitiveBox(tree, member));
        }
      }
    }

    this.gridFirstCell[grid] = this.cellStart.length;
    for (let c = 0; c < numberOfCells; c++) {
      this.cellStart.push(this.cellItems.length);
      const list = cells[c];
      if (list !== null && list !== undefined) {
        for (let i = 0; i < list.length; i++) {
          this.cellItems.push(list[i]!);
        }
      }
    }
    this.cellStart.push(this.cellItems.length);
    return grid;
  }

  private fitsInCell(tree: SpatialGroupTree, grid: number, group: number): boolean {
    if (!tree.isBounded(group)) {
      return false;
    }
    for (let k = 0; k < 3; k++) {
      if (tree.getGroupBox(group, 3 + k) - tree.getGroupBox(group, k) > this.gridCellSizes[3 * grid + k]!) {
        return false;
      }
    }
    return true;
  }

  /**
  Puts a member group into a grid: as one item when it fits in a cell (a sub
  grid when it is large enough), otherwise member by member
  */
  private insert(tree: SpatialGroupTree, grid: number, cells: Array<number[] | null>, group: number): void {
    if (this.fitsInCell(tree, grid, group)) {
      const box = [0, 0, 0, 0, 0, 0];
      if (tree.getPrimitiveCount(group) >= GridSpatialIndex.MINIMUM_ITEMS_FOR_SUB_GRID) {
        const subGrid = this.buildGrid(tree, group);
        for (let k = 0; k < 6; k++) {
          box[k] = this.gridBoxes[6 * subGrid + k]!;
        }
        this.insertItem(grid, cells, this.newItem(GridSpatialIndex.GRID_ITEM, subGrid), box);
      }
      else {
        for (let k = 0; k < 6; k++) {
          box[k] = tree.getGroupBox(group, k);
        }
        this.insertItem(grid, cells, this.newItem(GridSpatialIndex.GROUP_ITEM, group), box);
      }
      return;
    }

    for (let i = 0; i < tree.getNumberOfMembers(group); i++) {
      const member = tree.getMember(group, i);
      if (SpatialGroupTree.isGroupMember(member)) {
        this.insert(tree, grid, cells, SpatialGroupTree.memberGroup(member));
      }
      else {
        this.insertItem(grid, cells, this.newItem(GridSpatialIndex.PRIMITIVE_ITEM, member), GridSpatialIndex.primitiveBox(tree, member));
      }
    }
  }

  /** Lists an item in every cell its enlarged box overlaps */
  private insertItem(grid: number, cells: Array<number[] | null>, item: number, box: number[]): void {
    const padded = [0, 0, 0, 0, 0, 0];
    GridSpatialIndex.enlargedBox(box, padded);
    const resolution = this.gridResolution[grid]!;
    const lo = [0, 0, 0];
    const hi = [0, 0, 0];
    for (let k = 0; k < 3; k++) {
      lo[k] = GridSpatialIndex.cellCoordinate(padded[k]!, this.gridBoxes[6 * grid + k]!, this.gridCellSizes[3 * grid + k]!, resolution);
      hi[k] = GridSpatialIndex.cellCoordinate(padded[3 + k]!, this.gridBoxes[6 * grid + k]!, this.gridCellSizes[3 * grid + k]!, resolution);
    }
    for (let a = lo[0]!; a <= hi[0]!; a++) {
      for (let b = lo[1]!; b <= hi[1]!; b++) {
        for (let c = lo[2]!; c <= hi[2]!; c++) {
          const cell = (a * resolution + b) * resolution + c;
          let list = cells[cell];
          if (list === null || list === undefined) {
            list = [];
            cells[cell] = list;
          }
          list.push(item);
        }
      }
    }
  }

  public override getNumberOfPrimitives(): number {
    return this.numberOfPrimitives;
  }

  public override getWorkspaceSize(): number {
    return this.getNumberOfItems();
  }

  public getNumberOfGrids(): number {
    return this.gridResolution.length;
  }

  public getNumberOfItems(): number {
    return this.itemKind.length;
  }

  /** Cells per axis of a grid; grid 0 is the top level grid */
  public getResolution(grid: number): number {
    return this.gridResolution[grid]!;
  }

  /**
  Clips the segment [minimumT, maximumT] of the ray to a box, as RenderPark's
  box test does, and gives the parameter where the segment enters it
  @param entryT one-element holder for the entry parameter
  */
  private segmentEntry(boxes: number[], offset: number, ray: Ray, minimumT: number, maximumT: number, entryT: number[]): boolean {
    let nearT = minimumT;
    let farT = maximumT;
    for (let k = 0; k < 3; k++) {
      const origin = component(ray.getOrigin(), k);
      const direction = component(ray.getDirection(), k);
      if (direction === 0.0) {
        if (origin < boxes[offset + k]! || origin > boxes[offset + 3 + k]!) {
          return false;
        }
        continue;
      }
      let entry = (boxes[offset + k]! - origin) / direction;
      let exit = (boxes[offset + 3 + k]! - origin) / direction;
      if (entry > exit) {
        const swap = entry;
        entry = exit;
        exit = swap;
      }
      if (exit < nearT) {
        return false;
      }
      if (entry > nearT) {
        nearT = entry;
      }
      if (exit < farT) {
        farT = exit;
      }
      if (!(nearT <= farT * TOLERANCE_SCALE)) {
        return false;
      }
    }

    if (nearT === minimumT) {
      if (!(farT < maximumT)) {
        return false;
      }
      entryT[0] = farT;
      return true;
    }
    if (!(nearT < maximumT)) {
      return false;
    }
    entryT[0] = nearT;
    return true;
  }

  private testGroup(
    group: number,
    ray: Ray,
    minimumT: number,
    inOutBestT: number[],
    anyHit: boolean,
    test: PrimitiveRayTest,
    result: SpatialQueryResult[],
    tests: number[]
  ): boolean {
    if (this.groupBounded[group]!) {
      const p = [0, 0, 0];
      for (let k = 0; k < 3; k++) {
        p[k] = component(ray.getOrigin(), k) + minimumT * component(ray.getDirection(), k);
      }
      const entryT = [0.0];
      if (outOfBox(this.groupBoxes, 6 * group, p) && !this.segmentEntry(this.groupBoxes, 6 * group, ray, minimumT, inOutBestT[0]!, entryT)) {
        return false;
      }
    }

    let accepted = false;
    const t = [0.0];
    for (let i = this.groupFirstMember[group]!; i < this.groupFirstMember[group + 1]!; i++) {
      const member = this.groupMembers[i]!;
      let hit: boolean;
      if (SpatialGroupTree.isGroupMember(member)) {
        hit = this.testGroup(SpatialGroupTree.memberGroup(member), ray, minimumT, inOutBestT, anyHit, test, result, tests);
      }
      else {
        tests[0]!++;
        hit = test.test(member, minimumT, inOutBestT[0]!, t);
        if (hit) {
          inOutBestT[0] = t[0]!;
          result[0] = new SpatialQueryResult(member, t[0]!);
        }
      }
      if (hit) {
        accepted = true;
        if (anyHit) {
          return true;
        }
      }
    }
    return accepted;
  }

  private testItem(
    item: number,
    ray: Ray,
    minimumT: number,
    inOutBestT: number[],
    anyHit: boolean,
    test: PrimitiveRayTest,
    workspace: SpatialIndexWorkspace,
    result: SpatialQueryResult[],
    tests: number[]
  ): boolean {
    const reference = this.itemReference[item]!;
    const kind = this.itemKind[item]!;
    if (kind === GridSpatialIndex.PRIMITIVE_ITEM) {
      const t = [0.0];
      tests[0]!++;
      if (test.test(reference, minimumT, inOutBestT[0]!, t)) {
        inOutBestT[0] = t[0]!;
        result[0] = new SpatialQueryResult(reference, t[0]!);
        return true;
      }
      return false;
    }
    if (kind === GridSpatialIndex.GROUP_ITEM) {
      return this.testGroup(reference, ray, minimumT, inOutBestT, anyHit, test, result, tests);
    }
    const visits = [0];
    const hit = this.traverseGrid(reference, ray, minimumT, inOutBestT, anyHit, test, workspace, result, tests, visits);
    workspace.addNodeVisits(visits[0]!);
    return hit;
  }

  private traverseGrid(
    grid: number,
    ray: Ray,
    minimumT: number,
    inOutBestT: number[],
    anyHit: boolean,
    test: PrimitiveRayTest,
    workspace: SpatialIndexWorkspace,
    result: SpatialQueryResult[],
    tests: number[],
    visits: number[]
  ): boolean {
    const boxOffset = 6 * grid;
    const box = this.gridBoxes;
    const resolution = this.gridResolution[grid]!;
    const cellSizeOffset = 3 * grid;
    const cellSize = this.gridCellSizes;
    const mailbox = workspace.getNumberOfPrimitives() >= this.getNumberOfItems();
    const origin = ray.getOrigin();
    const rayDirection = ray.getDirection();

    // Where the ray enters the grid
    const t0 = [minimumT];
    const p = [0, 0, 0];
    for (let k = 0; k < 3; k++) {
      p[k] = component(origin, k) + t0[0]! * component(rayDirection, k);
    }
    if (outOfBox(box, boxOffset, p)) {
      if (!this.segmentEntry(box, boxOffset, ray, minimumT, inOutBestT[0]!, t0)) {
        return false;
      }
      for (let k = 0; k < 3; k++) {
        p[k] = component(origin, k) + t0[0]! * component(rayDirection, k);
      }
    }

    // The cell it enters, and the parameters of the next cell boundaries
    const g = [0, 0, 0];
    const step = [0, 0, 0];
    const out = [0, 0, 0];
    const tDelta = [0, 0, 0];
    const tNext = [0, 0, 0];
    for (let k = 0; k < 3; k++) {
      g[k] = GridSpatialIndex.cellCoordinate(p[k]!, box[boxOffset + k]!, cellSize[cellSizeOffset + k]!, resolution);
      const direction = component(rayDirection, k);
      if (globalThis.Math.abs(direction) > EPSILON) {
        if (direction > 0.0) {
          tDelta[k] = cellSize[cellSizeOffset + k]! / direction;
          tNext[k] = t0[0]! + ((g[k]! + 1) * cellSize[cellSizeOffset + k]! + box[boxOffset + k]! - p[k]!) / direction;
          step[k] = 1;
          out[k] = resolution;
        }
        else {
          tDelta[k] = cellSize[cellSizeOffset + k]! / -direction;
          tNext[k] = t0[0]! + (g[k]! * cellSize[cellSizeOffset + k]! + box[boxOffset + k]! - p[k]!) / direction;
          step[k] = -1;
          out[k] = -1;
        }
      }
      else {
        tDelta[k] = 0.0;
        tNext[k] = NO_BOUNDARY;
        step[k] = 0;
        out[k] = 0;
      }
    }

    const firstCell = this.gridFirstCell[grid]!;
    let accepted = false;
    let inGrid: boolean;
    let t = t0[0]!;
    do {
      visits[0]!++;
      const cell = firstCell + (g[0]! * resolution + g[1]!) * resolution + g[2]!;
      for (let i = this.cellStart[cell]!; i < this.cellStart[cell + 1]!; i++) {
        const item = this.cellItems[i]!;
        if (!mailbox || workspace.firstVisit(item)) {
          if (this.testItem(item, ray, minimumT, inOutBestT, anyHit, test, workspace, result, tests)) {
            accepted = true;
          }
        }
      }

      let k = 2;
      if (tNext[0]! <= tNext[1]! && tNext[0]! <= tNext[2]!) {
        k = 0;
      }
      else if (tNext[1]! <= tNext[2]!) {
        k = 1;
      }
      g[k]! += step[k]!;
      t = tNext[k]!;
      tNext[k]! += tDelta[k]!;
      inGrid = g[k] !== out[k];
    } while (inGrid && t <= inOutBestT[0]!);

    return accepted;
  }

  public override query(
    ray: Ray,
    minimumT: number,
    maximumT: number,
    mode: SpatialQueryMode,
    test: PrimitiveRayTest,
    workspace: SpatialIndexWorkspace
  ): SpatialQueryResult {
    const result = [new SpatialQueryResult()];
    if (this.getNumberOfItems() === 0) {
      return result[0]!;
    }
    if (workspace.getNumberOfPrimitives() >= this.getNumberOfItems()) {
      workspace.beginMailboxQuery();
    }
    const bestT = [maximumT];
    const tests = [0];
    const visits = [0];
    this.traverseGrid(0, ray, minimumT, bestT, mode === SpatialQueryMode.ANY_HIT, test, workspace, result, tests, visits);
    workspace.addNodeVisits(visits[0]!);
    workspace.addPrimitiveTests(tests[0]!);
    return result[0]!;
  }
}
