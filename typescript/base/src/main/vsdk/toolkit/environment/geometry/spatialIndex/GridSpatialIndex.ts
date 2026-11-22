//= References:                                                             =
//= [SNYD1987] Snyder, John M. and Barr, Alan H. "Ray Tracing Complex       =
//=     Models Containing Surface Tessellations". SIGGRAPH '87, Computer    =
//=     Graphics 21(4), 1987, pp. 119-128.                                  =
//=     PDF: doc/references/[SNYD1987]_*.pdf                                =

import { ArrayList } from "../../../../../java/util/ArrayList.js";
import type { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import type { Ray } from "../element/Ray.js";
import type { PrimitiveRayTest } from "./PrimitiveRayTest.js";
import { SpatialGroupTree } from "./SpatialGroupTree.js";
import { SpatialIndex } from "./SpatialIndex.js";
import type { SpatialIndexWorkspace } from "./SpatialIndexWorkspace.js";
import { SpatialQueryMode } from "./SpatialQueryMode.js";
import { SpatialQueryResult } from "./SpatialQueryResult.js";

// Smallest cell size and direction component the traversal divides by, and
// the smallest enlargement of a box side (RenderPark's EPSILON)
const EPSILON = 1e-6;

// Relative tolerance of the far end in a box/segment clip
const TOLERANCE_SCALE = 1.0 + 1e-6;

// Parameter of the next cell boundary along an axis the ray does not cross
const NO_BOUNDARY = 1e300;

function component(v: Vector3Dd, k: number): number {
    return k === 0 ? v.x() : k === 1 ? v.y() : v.z();
}

function outOfBox(box: ArrayLike<number>, boxOffset: number, p: ArrayLike<number>): boolean {
    for (let k = 0; k < 3; k++) {
        if (p[k]! < box[boxOffset + k]! || p[k]! > box[boxOffset + 3 + k]!) {
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

Correspondence with [SNYD1987]:
- section 3.1: a grid is a box divided in cells, each non-empty cell with
  the list of the objects inside it; section 3.2: lists and grids nest, a
  grid or list being an object of another one (here a sub grid or a group
  item);
- Figure 6 (generic object engrid): an item goes to every cell its box
  overlaps. The clipping of a primitive's box to each cell (Figure 7) is not
  done;
- Figures 9 and 10 (ray / 3D grid): the cell walk, with the parameters of
  the next boundary along each axis and their increments, stepping along the
  smallest; double precision, as the paper's implementation (section 5.2);
- Figure 11 (an object in several cells): the paper accepts a hit only if it
  lies inside the current cell; here the walk instead goes on until it
  enters a cell beyond the closest hit, which gives the same closest hit;
- not in the paper: the resolution floor(count^(1/3) + 1) (the paper's
  modeller chooses the divisions; this one is RenderPark's); the cell list
  box flags and the ray box cull of Figures 12 and 13 are not done.

Shared, read-only after construction: the index keeps its own copy of the
tree data it needs, and a query writes only to the workspace and the test
it is given.

C++ output and in/out parameters (`double &`, `long int &`, `double *`) are
one-element holders here.
*/
export class GridSpatialIndex extends SpatialIndex {
    public static readonly MINIMUM_ITEMS_FOR_SUB_GRID = 10;
    public static readonly DELTA_BOUND_FACTOR = 1e-4;

    private static readonly PRIMITIVE_ITEM = 0;
    private static readonly GROUP_ITEM = 1;
    private static readonly GRID_ITEM = 2;

    private readonly numberOfPrimitives: number;

    // Items, dense ids: kind and the primitive, group or grid they refer to
    private readonly itemKind: ArrayList<number> = new ArrayList<number>();
    private readonly itemReference: ArrayList<number> = new ArrayList<number>();

    // Grids: padded box (6 values), cells per axis, cell size (3 values),
    // first entry of the grid in cellStart (cells + 1 entries per grid)
    private readonly gridBoxes: ArrayList<number> = new ArrayList<number>();
    private readonly gridResolution: ArrayList<number> = new ArrayList<number>();
    private readonly gridCellSizes: ArrayList<number> = new ArrayList<number>();
    private readonly gridFirstCell: ArrayList<number> = new ArrayList<number>();
    private readonly cellStart: ArrayList<number> = new ArrayList<number>();
    private readonly cellItems: ArrayList<number> = new ArrayList<number>();

    // Groups used as items: padded box (6 values), bounded flag, members
    // (as in SpatialGroupTree.getMember) from groupFirstMember
    private readonly groupBoxes: ArrayList<number> = new ArrayList<number>();
    private readonly groupBounded: ArrayList<number> = new ArrayList<number>();
    private readonly groupFirstMember: ArrayList<number> = new ArrayList<number>();
    private readonly groupMembers: ArrayList<number> = new ArrayList<number>();

    // Flat copies of the arrays read by the query (TypeScript only: the
    // query reads them as the C++ query reads the ArrayList storage)
    private gridBoxArray: Float64Array = new Float64Array(0);
    private groupBoxArray: Float64Array = new Float64Array(0);

    /**
    @param tree the groups and the primitive boxes, unpadded (the index
    pads them by DELTA_BOUND_FACTOR); its root group must have a box, and
    only the primitives that are members of a group are indexed
    */
    public constructor(tree: SpatialGroupTree) {
        super();
        this.numberOfPrimitives = tree.getNumberOfPrimitives();
        const box: number[] = [0, 0, 0, 0, 0, 0];
        const padded: number[] = [0, 0, 0, 0, 0, 0];
        for (let g = 0; g < tree.getNumberOfGroups(); g++) {
            for (let k = 0; k < 6; k++) {
                box[k] = tree.getGroupBox(g, k);
            }
            GridSpatialIndex.enlargedBox(box, padded);
            for (let k = 0; k < 6; k++) {
                this.groupBoxes.add(padded[k]!);
            }
            this.groupBounded.add(tree.isBounded(g) ? 1 : 0);
            this.groupFirstMember.add(this.groupMembers.size());
            for (let i = 0; i < tree.getNumberOfMembers(g); i++) {
                this.groupMembers.add(tree.getMember(g, i));
            }
        }
        this.groupFirstMember.add(this.groupMembers.size());

        this.buildGrid(tree, 0);
        this.gridBoxArray = Float64Array.from(this.gridBoxes.toArray());
        this.groupBoxArray = Float64Array.from(this.groupBoxes.toArray());
    }

    /**
    The box enlarged on each side by DELTA_BOUND_FACTOR of its size along that
    axis, and by at least EPSILON
    */
    private static enlargedBox(box: ArrayLike<number>, out: number[]): void {
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
        let v = size < EPSILON ? 0 : Math.trunc((p - lo) / size);
        if (v < 0) {
            v = 0;
        }
        if (v >= resolution) {
            v = resolution - 1;
        }
        return v;
    }

    private newItem(kind: number, reference: number): number {
        this.itemKind.add(kind);
        this.itemReference.add(reference);
        return this.itemKind.size() - 1;
    }

    /**
    Builds the grid over a group (and, recursively, its sub grids) and returns
    its id
    */
    private buildGrid(tree: SpatialGroupTree, group: number): number {
        const grid = this.gridResolution.size();
        const count = tree.getPrimitiveCount(group);
        const resolution = Math.trunc(Math.floor(Math.pow(count, 0.33333) + 1.0));

        for (let k = 0; k < 6; k++) {
            this.gridBoxes.add(this.groupBoxes.get(6 * group + k));
        }
        this.gridResolution.add(resolution);
        for (let k = 0; k < 3; k++) {
            this.gridCellSizes.add(
                (this.gridBoxes.get(6 * grid + 3 + k) - this.gridBoxes.get(6 * grid + k)) / resolution,
            );
        }
        this.gridFirstCell.add(0);

        const numberOfCells = resolution * resolution * resolution;
        const cells: (ArrayList<number> | null)[] = new Array<ArrayList<number> | null>(numberOfCells);
        for (let c = 0; c < numberOfCells; c++) {
            cells[c] = null;
        }

        if (resolution === 1) {
            this.insert(tree, grid, cells, group);
        } else {
            // The group of a grid never fits in one of its cells (unless its box
            // is degenerate, where this avoids an endless recursion)
            for (let i = 0; i < tree.getNumberOfMembers(group); i++) {
                const member = tree.getMember(group, i);
                if (SpatialGroupTree.isGroupMember(member)) {
                    this.insert(tree, grid, cells, SpatialGroupTree.memberGroup(member));
                } else {
                    const box: number[] = [0, 0, 0, 0, 0, 0];
                    for (let k = 0; k < 6; k++) {
                        box[k] = tree.getPrimitiveBoxes().get(6 * member + k);
                    }
                    this.insertItem(grid, cells, this.newItem(GridSpatialIndex.PRIMITIVE_ITEM, member), box);
                }
            }
        }

        this.gridFirstCell.set(grid, this.cellStart.size());
        for (let c = 0; c < numberOfCells; c++) {
            this.cellStart.add(this.cellItems.size());
            const list = cells[c] ?? null;
            if (list !== null) {
                for (let i = 0; i < list.size(); i++) {
                    this.cellItems.add(list.get(i));
                }
            }
        }
        this.cellStart.add(this.cellItems.size());
        return grid;
    }

    private fitsInCell(tree: SpatialGroupTree, grid: number, group: number): boolean {
        if (!tree.isBounded(group)) {
            return false;
        }
        for (let k = 0; k < 3; k++) {
            if (tree.getGroupBox(group, 3 + k) - tree.getGroupBox(group, k) > this.gridCellSizes.get(3 * grid + k)) {
                return false;
            }
        }
        return true;
    }

    /**
    Puts a member group into a grid: as one item when it fits in a cell (a sub
    grid when it is large enough), otherwise member by member
    */
    private insert(tree: SpatialGroupTree, grid: number, cells: (ArrayList<number> | null)[], group: number): void {
        if (this.fitsInCell(tree, grid, group)) {
            const box: number[] = [0, 0, 0, 0, 0, 0];
            if (tree.getPrimitiveCount(group) >= GridSpatialIndex.MINIMUM_ITEMS_FOR_SUB_GRID) {
                const subGrid = this.buildGrid(tree, group);
                for (let k = 0; k < 6; k++) {
                    box[k] = this.gridBoxes.get(6 * subGrid + k);
                }
                this.insertItem(grid, cells, this.newItem(GridSpatialIndex.GRID_ITEM, subGrid), box);
            } else {
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
            } else {
                const box: number[] = [0, 0, 0, 0, 0, 0];
                for (let k = 0; k < 6; k++) {
                    box[k] = tree.getPrimitiveBoxes().get(6 * member + k);
                }
                this.insertItem(grid, cells, this.newItem(GridSpatialIndex.PRIMITIVE_ITEM, member), box);
            }
        }
    }

    /** Lists an item in every cell its enlarged box overlaps */
    private insertItem(grid: number, cells: (ArrayList<number> | null)[], item: number, box: ArrayLike<number>): void {
        const padded: number[] = [0, 0, 0, 0, 0, 0];
        GridSpatialIndex.enlargedBox(box, padded);
        const resolution = this.gridResolution.get(grid);
        const lo: number[] = [0, 0, 0];
        const hi: number[] = [0, 0, 0];
        for (let k = 0; k < 3; k++) {
            lo[k] = GridSpatialIndex.cellCoordinate(
                padded[k]!,
                this.gridBoxes.get(6 * grid + k),
                this.gridCellSizes.get(3 * grid + k),
                resolution,
            );
            hi[k] = GridSpatialIndex.cellCoordinate(
                padded[3 + k]!,
                this.gridBoxes.get(6 * grid + k),
                this.gridCellSizes.get(3 * grid + k),
                resolution,
            );
        }
        for (let a = lo[0]!; a <= hi[0]!; a++) {
            for (let b = lo[1]!; b <= hi[1]!; b++) {
                for (let c = lo[2]!; c <= hi[2]!; c++) {
                    const cell = (a * resolution + b) * resolution + c;
                    if (cells[cell] === null) {
                        cells[cell] = new ArrayList<number>();
                    }
                    cells[cell]!.add(item);
                }
            }
        }
    }

    public getNumberOfPrimitives(): number {
        return this.numberOfPrimitives;
    }

    public getWorkspaceSize(): number {
        return this.getNumberOfItems();
    }

    public getNumberOfGrids(): number {
        return this.gridResolution.size();
    }

    public getNumberOfItems(): number {
        return this.itemKind.size();
    }

    /** Cells per axis of a grid; grid 0 is the top level grid */
    public getResolution(grid: number): number {
        return this.gridResolution.get(grid);
    }

    /**
    Clips the segment [minimumT, maximumT] of the ray to a box, as RenderPark's
    box test does, and gives the parameter where the segment enters it
    @param entryT one-element holder (C++ `double *`)
    */
    private segmentEntry(
        box: ArrayLike<number>,
        boxOffset: number,
        ray: Ray,
        minimumT: number,
        maximumT: number,
        entryT: number[],
    ): boolean {
        let nearT = minimumT;
        let farT = maximumT;
        for (let k = 0; k < 3; k++) {
            const origin = component(ray.getOrigin(), k);
            const direction = component(ray.getDirection(), k);
            if (direction === 0.0) {
                if (origin < box[boxOffset + k]! || origin > box[boxOffset + 3 + k]!) {
                    return false;
                }
                continue;
            }
            let entry = (box[boxOffset + k]! - origin) / direction;
            let exit = (box[boxOffset + 3 + k]! - origin) / direction;
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

    private testGroup<Test extends PrimitiveRayTest>(
        group: number,
        ray: Ray,
        minimumT: number,
        inOutBestT: number[],
        anyHit: boolean,
        test: Test,
        result: SpatialQueryResult[],
        tests: number[],
    ): boolean {
        if (this.groupBounded.get(group) !== 0) {
            const box = this.groupBoxArray;
            const offset = 6 * group;
            const p: number[] = [0, 0, 0];
            for (let k = 0; k < 3; k++) {
                p[k] = component(ray.getOrigin(), k) + minimumT * component(ray.getDirection(), k);
            }
            const entryT: number[] = [0];
            if (outOfBox(box, offset, p) && !this.segmentEntry(box, offset, ray, minimumT, inOutBestT[0]!, entryT)) {
                return false;
            }
        }

        let accepted = false;
        const t: number[] = [0];
        for (let i = this.groupFirstMember.get(group); i < this.groupFirstMember.get(group + 1); i++) {
            const member = this.groupMembers.get(i);
            let hit: boolean;
            if (SpatialGroupTree.isGroupMember(member)) {
                hit = this.testGroup(
                    SpatialGroupTree.memberGroup(member),
                    ray,
                    minimumT,
                    inOutBestT,
                    anyHit,
                    test,
                    result,
                    tests,
                );
            } else {
                tests[0] = tests[0]! + 1;
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

    private testItem<Test extends PrimitiveRayTest>(
        item: number,
        ray: Ray,
        minimumT: number,
        inOutBestT: number[],
        anyHit: boolean,
        test: Test,
        workspace: SpatialIndexWorkspace,
        result: SpatialQueryResult[],
        tests: number[],
    ): boolean {
        const reference = this.itemReference.get(item);
        if (this.itemKind.get(item) === GridSpatialIndex.PRIMITIVE_ITEM) {
            const t: number[] = [0];
            tests[0] = tests[0]! + 1;
            if (test.test(reference, minimumT, inOutBestT[0]!, t)) {
                inOutBestT[0] = t[0]!;
                result[0] = new SpatialQueryResult(reference, t[0]!);
                return true;
            }
            return false;
        }
        if (this.itemKind.get(item) === GridSpatialIndex.GROUP_ITEM) {
            return this.testGroup(reference, ray, minimumT, inOutBestT, anyHit, test, result, tests);
        }
        const visits: number[] = [0];
        const hit = this.traverseGrid(
            reference,
            ray,
            minimumT,
            inOutBestT,
            anyHit,
            test,
            workspace,
            result,
            tests,
            visits,
        );
        workspace.addNodeVisits(visits[0]!);
        return hit;
    }

    /**
    Walk of one grid, [SNYD1987] Figures 9 and 10: enter the grid, then visit
    the cells the ray crosses in order, stepping each time along the axis whose
    next cell boundary is closest.
    */
    private traverseGrid<Test extends PrimitiveRayTest>(
        grid: number,
        ray: Ray,
        minimumT: number,
        inOutBestT: number[],
        anyHit: boolean,
        test: Test,
        workspace: SpatialIndexWorkspace,
        result: SpatialQueryResult[],
        tests: number[],
        visits: number[],
    ): boolean {
        const box = this.gridBoxArray;
        const boxOffset = 6 * grid;
        const resolution = this.gridResolution.get(grid);
        const cellSize: number[] = [
            this.gridCellSizes.get(3 * grid),
            this.gridCellSizes.get(3 * grid + 1),
            this.gridCellSizes.get(3 * grid + 2),
        ];
        const mailbox = workspace.getNumberOfPrimitives() >= this.getNumberOfItems();

        // Where the ray enters the grid
        const t0Holder: number[] = [minimumT];
        const p: number[] = [0, 0, 0];
        for (let k = 0; k < 3; k++) {
            p[k] = component(ray.getOrigin(), k) + t0Holder[0]! * component(ray.getDirection(), k);
        }
        if (outOfBox(box, boxOffset, p)) {
            if (!this.segmentEntry(box, boxOffset, ray, minimumT, inOutBestT[0]!, t0Holder)) {
                return false;
            }
            for (let k = 0; k < 3; k++) {
                p[k] = component(ray.getOrigin(), k) + t0Holder[0]! * component(ray.getDirection(), k);
            }
        }
        let t0 = t0Holder[0]!;

        // The cell it enters, and the parameters of the next cell boundaries
        const g: number[] = [0, 0, 0];
        const step: number[] = [0, 0, 0];
        const out: number[] = [0, 0, 0];
        const tDelta: number[] = [0, 0, 0];
        const tNext: number[] = [0, 0, 0];
        for (let k = 0; k < 3; k++) {
            g[k] = GridSpatialIndex.cellCoordinate(p[k]!, box[boxOffset + k]!, cellSize[k]!, resolution);
            const direction = component(ray.getDirection(), k);
            if (Math.abs(direction) > EPSILON) {
                if (direction > 0.0) {
                    tDelta[k] = cellSize[k]! / direction;
                    tNext[k] = t0 + ((g[k]! + 1) * cellSize[k]! + box[boxOffset + k]! - p[k]!) / direction;
                    step[k] = 1;
                    out[k] = resolution;
                } else {
                    tDelta[k] = cellSize[k]! / -direction;
                    tNext[k] = t0 + (g[k]! * cellSize[k]! + box[boxOffset + k]! - p[k]!) / direction;
                    step[k] = -1;
                    out[k] = -1;
                }
            } else {
                tDelta[k] = 0.0;
                tNext[k] = NO_BOUNDARY;
                step[k] = 0;
                out[k] = 0;
            }
        }

        const firstCell = this.gridFirstCell.get(grid);
        let accepted = false;
        let inGrid: boolean;
        do {
            visits[0] = visits[0]! + 1;
            const cell = firstCell + (g[0]! * resolution + g[1]!) * resolution + g[2]!;
            for (let i = this.cellStart.get(cell); i < this.cellStart.get(cell + 1); i++) {
                const item = this.cellItems.get(i);
                if (!mailbox || workspace.firstVisit(item)) {
                    if (this.testItem(item, ray, minimumT, inOutBestT, anyHit, test, workspace, result, tests)) {
                        accepted = true;
                    }
                }
            }

            let k = 2;
            if (tNext[0]! <= tNext[1]! && tNext[0]! <= tNext[2]!) {
                k = 0;
            } else if (tNext[1]! <= tNext[2]!) {
                k = 1;
            }
            g[k] = g[k]! + step[k]!;
            t0 = tNext[k]!;
            tNext[k] = tNext[k]! + tDelta[k]!;
            inGrid = g[k] !== out[k];
        } while (inGrid && t0 <= inOutBestT[0]!);

        return accepted;
    }

    public query(
        ray: Ray,
        minimumT: number,
        maximumT: number,
        mode: SpatialQueryMode,
        test: PrimitiveRayTest,
        workspace: SpatialIndexWorkspace,
    ): SpatialQueryResult {
        return this.queryWith(ray, minimumT, maximumT, mode, test, workspace);
    }

    /**
    `query` with the exact test given by its own type: the same traversal.
    In C++ it is a template, so that the compiler can inline a final test
    class into the cell loop; here it is kept for the same callers.
    */
    public queryWith<Test extends PrimitiveRayTest>(
        ray: Ray,
        minimumT: number,
        maximumT: number,
        mode: SpatialQueryMode,
        test: Test,
        workspace: SpatialIndexWorkspace,
    ): SpatialQueryResult {
        const result: SpatialQueryResult[] = [new SpatialQueryResult()];
        if (this.getNumberOfItems() === 0) {
            return result[0]!;
        }
        if (workspace.getNumberOfPrimitives() >= this.getNumberOfItems()) {
            workspace.beginMailboxQuery();
        }
        const bestT: number[] = [maximumT];
        const tests: number[] = [0];
        const visits: number[] = [0];
        this.traverseGrid(
            0,
            ray,
            minimumT,
            bestT,
            mode === SpatialQueryMode.ANY_HIT,
            test,
            workspace,
            result,
            tests,
            visits,
        );
        workspace.addNodeVisits(visits[0]!);
        workspace.addPrimitiveTests(tests[0]!);
        return result[0]!;
    }
}
