//= References:                                                             =
//= [SNYD1987] Snyder, John M. and Barr, Alan H. "Ray Tracing Complex       =
//=     Models Containing Surface Tessellations". SIGGRAPH '87, Computer    =
//=     Graphics 21(4), 1987, pp. 119-128.                                  =

package vsdk.toolkit.environment.geometry.spatialIndex;

import java.util.ArrayList;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.element.Ray;

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

Java adaptation: the tables are built in lists and frozen into primitive
arrays at the end of the construction; the result and the counters that the
C++ port passes by reference travel in a `QueryState`.
*/
public final class GridSpatialIndex extends SpatialIndex {
    public static final int MINIMUM_ITEMS_FOR_SUB_GRID = 10;
    public static final double DELTA_BOUND_FACTOR = 1e-4;

    private static final int PRIMITIVE_ITEM = 0;
    private static final int GROUP_ITEM = 1;
    private static final int GRID_ITEM = 2;

    // Smallest cell size and direction component the traversal divides by, and
    // the smallest enlargement of a box side (RenderPark's EPSILON)
    private static final double EPSILON = 1e-6;

    // Relative tolerance of the far end in a box/segment clip
    private static final double TOLERANCE_SCALE = 1.0 + 1e-6;

    // Parameter of the next cell boundary along an axis the ray does not cross
    private static final double NO_BOUNDARY = 1e300;

    private final int numberOfPrimitives;

    // Construction lists, frozen into the arrays below
    private ArrayList<Integer> itemKindList;
    private ArrayList<Integer> itemReferenceList;
    private ArrayList<Double> gridBoxList;
    private ArrayList<Integer> gridResolutionList;
    private ArrayList<Double> gridCellSizeList;
    private ArrayList<Integer> gridFirstCellList;
    private ArrayList<Integer> cellStartList;
    private ArrayList<Integer> cellItemList;

    // Items, dense ids: kind and the primitive, group or grid they refer to
    private final int[] itemKind;
    private final int[] itemReference;

    // Grids: padded box (6 values), cells per axis, cell size (3 values),
    // first entry of the grid in cellStart (cells + 1 entries per grid)
    private final double[] gridBoxes;
    private final int[] gridResolution;
    private final double[] gridCellSizes;
    private final int[] gridFirstCell;
    private final int[] cellStart;
    private final int[] cellItems;

    // Groups used as items: padded box (6 values), bounded flag, members
    // (as in SpatialGroupTree.getMember) from groupFirstMember
    private final double[] groupBoxes;
    private final boolean[] groupBounded;
    private final int[] groupFirstMember;
    private final int[] groupMembers;

    /** What one query accumulates (the C++ port passes it by reference) */
    private static final class QueryState {
        SpatialQueryResult result;
        long tests;
        final double[] hitT;

        QueryState(double[] inHitT) {
            result = null;
            tests = 0;
            hitT = inHitT;
        }
    }

    private static double component(Vector3Dd v, int k) {
        return k == 0 ? v.x() : (k == 1 ? v.y() : v.z());
    }

    private static boolean outOfBox(double[] box, int offset, double[] p) {
        for ( int k = 0; k < 3; k++ ) {
            if ( p[k] < box[offset + k] || p[k] > box[offset + 3 + k] ) {
                return true;
            }
        }
        return false;
    }

    private static int[] toIntArray(ArrayList<Integer> list) {
        int[] array = new int[list.size()];
        for ( int i = 0; i < array.length; i++ ) {
            array[i] = list.get(i);
        }
        return array;
    }

    private static double[] toDoubleArray(ArrayList<Double> list) {
        double[] array = new double[list.size()];
        for ( int i = 0; i < array.length; i++ ) {
            array[i] = list.get(i);
        }
        return array;
    }

    /**
    @param tree the groups and the primitive boxes, unpadded (the index
    pads them by DELTA_BOUND_FACTOR); its root group must have a box, and
    only the primitives that are members of a group are indexed
    */
    public GridSpatialIndex(SpatialGroupTree tree) {
        numberOfPrimitives = tree.getNumberOfPrimitives();
        itemKindList = new ArrayList<>();
        itemReferenceList = new ArrayList<>();
        gridBoxList = new ArrayList<>();
        gridResolutionList = new ArrayList<>();
        gridCellSizeList = new ArrayList<>();
        gridFirstCellList = new ArrayList<>();
        cellStartList = new ArrayList<>();
        cellItemList = new ArrayList<>();

        final int numberOfGroups = tree.getNumberOfGroups();
        groupBoxes = new double[6 * numberOfGroups];
        groupBounded = new boolean[numberOfGroups];
        groupFirstMember = new int[numberOfGroups + 1];
        ArrayList<Integer> members = new ArrayList<>();
        double[] box = new double[6];
        double[] padded = new double[6];
        for ( int g = 0; g < numberOfGroups; g++ ) {
            for ( int k = 0; k < 6; k++ ) {
                box[k] = tree.getGroupBox(g, k);
            }
            enlargedBox(box, 0, padded);
            for ( int k = 0; k < 6; k++ ) {
                groupBoxes[6 * g + k] = padded[k];
            }
            groupBounded[g] = tree.isBounded(g);
            groupFirstMember[g] = members.size();
            for ( int i = 0; i < tree.getNumberOfMembers(g); i++ ) {
                members.add(tree.getMember(g, i));
            }
        }
        groupFirstMember[numberOfGroups] = members.size();
        groupMembers = toIntArray(members);

        buildGrid(tree, 0);

        itemKind = toIntArray(itemKindList);
        itemReference = toIntArray(itemReferenceList);
        gridBoxes = toDoubleArray(gridBoxList);
        gridResolution = toIntArray(gridResolutionList);
        gridCellSizes = toDoubleArray(gridCellSizeList);
        gridFirstCell = toIntArray(gridFirstCellList);
        cellStart = toIntArray(cellStartList);
        cellItems = toIntArray(cellItemList);
        itemKindList = null;
        itemReferenceList = null;
        gridBoxList = null;
        gridResolutionList = null;
        gridCellSizeList = null;
        gridFirstCellList = null;
        cellStartList = null;
        cellItemList = null;
    }

    /**
    The box enlarged on each side by DELTA_BOUND_FACTOR of its size along that
    axis, and by at least EPSILON
    */
    private static void enlargedBox(double[] box, int offset, double[] out) {
        for ( int k = 0; k < 3; k++ ) {
            double delta = (box[offset + 3 + k] - box[offset + k]) * DELTA_BOUND_FACTOR;
            if ( delta < EPSILON ) {
                delta = EPSILON;
            }
            out[k] = box[offset + k] - delta;
            out[3 + k] = box[offset + 3 + k] + delta;
        }
    }

    /** Index of the cell holding coordinate p along one axis, clamped to the grid */
    private static int cellCoordinate(double p, double lo, double size, int resolution) {
        int v = size < EPSILON ? 0 : (int)((p - lo) / size);
        if ( v < 0 ) {
            v = 0;
        }
        if ( v >= resolution ) {
            v = resolution - 1;
        }
        return v;
    }

    private int newItem(int kind, int reference) {
        itemKindList.add(kind);
        itemReferenceList.add(reference);
        return itemKindList.size() - 1;
    }

    /**
    Builds the grid over a group (and, recursively, its sub grids) and returns
    its id
    */
    private int buildGrid(SpatialGroupTree tree, int group) {
        final int grid = gridResolutionList.size();
        final int count = tree.getPrimitiveCount(group);
        final int resolution = (int)Math.floor(Math.pow((double)count, 0.33333) + 1.0);

        for ( int k = 0; k < 6; k++ ) {
            gridBoxList.add(groupBoxes[6 * group + k]);
        }
        gridResolutionList.add(resolution);
        for ( int k = 0; k < 3; k++ ) {
            gridCellSizeList.add((gridBoxList.get(6 * grid + 3 + k) - gridBoxList.get(6 * grid + k)) / resolution);
        }
        gridFirstCellList.add(0);

        final int numberOfCells = resolution * resolution * resolution;
        ArrayList<ArrayList<Integer>> cells = new ArrayList<>(numberOfCells);
        for ( int c = 0; c < numberOfCells; c++ ) {
            cells.add(null);
        }

        if ( resolution == 1 ) {
            insert(tree, grid, cells, group);
        }
        else {
            // The group of a grid never fits in one of its cells (unless its box
            // is degenerate, where this avoids an endless recursion)
            for ( int i = 0; i < tree.getNumberOfMembers(group); i++ ) {
                final int member = tree.getMember(group, i);
                if ( SpatialGroupTree.isGroupMember(member) ) {
                    insert(tree, grid, cells, SpatialGroupTree.memberGroup(member));
                }
                else {
                    insertItem(grid, cells, newItem(PRIMITIVE_ITEM, member), tree.getPrimitiveBoxes(), 6 * member);
                }
            }
        }

        gridFirstCellList.set(grid, cellStartList.size());
        for ( int c = 0; c < numberOfCells; c++ ) {
            cellStartList.add(cellItemList.size());
            if ( cells.get(c) != null ) {
                cellItemList.addAll(cells.get(c));
                cells.set(c, null);
            }
        }
        cellStartList.add(cellItemList.size());
        return grid;
    }

    private boolean fitsInCell(SpatialGroupTree tree, int grid, int group) {
        if ( !tree.isBounded(group) ) {
            return false;
        }
        for ( int k = 0; k < 3; k++ ) {
            if ( tree.getGroupBox(group, 3 + k) - tree.getGroupBox(group, k) > gridCellSizeList.get(3 * grid + k) ) {
                return false;
            }
        }
        return true;
    }

    /**
    Puts a member group into a grid: as one item when it fits in a cell (a sub
    grid when it is large enough), otherwise member by member
    */
    private void insert(SpatialGroupTree tree, int grid, ArrayList<ArrayList<Integer>> cells, int group) {
        if ( fitsInCell(tree, grid, group) ) {
            double[] box = new double[6];
            if ( tree.getPrimitiveCount(group) >= MINIMUM_ITEMS_FOR_SUB_GRID ) {
                final int subGrid = buildGrid(tree, group);
                for ( int k = 0; k < 6; k++ ) {
                    box[k] = gridBoxList.get(6 * subGrid + k);
                }
                insertItem(grid, cells, newItem(GRID_ITEM, subGrid), box, 0);
            }
            else {
                for ( int k = 0; k < 6; k++ ) {
                    box[k] = tree.getGroupBox(group, k);
                }
                insertItem(grid, cells, newItem(GROUP_ITEM, group), box, 0);
            }
            return;
        }

        for ( int i = 0; i < tree.getNumberOfMembers(group); i++ ) {
            final int member = tree.getMember(group, i);
            if ( SpatialGroupTree.isGroupMember(member) ) {
                insert(tree, grid, cells, SpatialGroupTree.memberGroup(member));
            }
            else {
                insertItem(grid, cells, newItem(PRIMITIVE_ITEM, member), tree.getPrimitiveBoxes(), 6 * member);
            }
        }
    }

    /** Lists an item in every cell its enlarged box overlaps */
    private void insertItem(int grid, ArrayList<ArrayList<Integer>> cells, int item, double[] box, int offset) {
        double[] padded = new double[6];
        enlargedBox(box, offset, padded);
        final int resolution = gridResolutionList.get(grid);
        int[] lo = new int[3];
        int[] hi = new int[3];
        for ( int k = 0; k < 3; k++ ) {
            lo[k] = cellCoordinate(padded[k], gridBoxList.get(6 * grid + k), gridCellSizeList.get(3 * grid + k), resolution);
            hi[k] = cellCoordinate(padded[3 + k], gridBoxList.get(6 * grid + k), gridCellSizeList.get(3 * grid + k), resolution);
        }
        for ( int a = lo[0]; a <= hi[0]; a++ ) {
            for ( int b = lo[1]; b <= hi[1]; b++ ) {
                for ( int c = lo[2]; c <= hi[2]; c++ ) {
                    final int cell = (a * resolution + b) * resolution + c;
                    if ( cells.get(cell) == null ) {
                        cells.set(cell, new ArrayList<>());
                    }
                    cells.get(cell).add(item);
                }
            }
        }
    }

    @Override
    public int getNumberOfPrimitives() {
        return numberOfPrimitives;
    }

    @Override
    public int getWorkspaceSize() {
        return getNumberOfItems();
    }

    public int getNumberOfGrids() {
        return gridResolution.length;
    }

    public int getNumberOfItems() {
        return itemKind.length;
    }

    /** Cells per axis of a grid; grid 0 is the top level grid */
    public int getResolution(int grid) {
        return gridResolution[grid];
    }

    /**
    Clips the segment [minimumT, maximumT] of the ray to a box, as RenderPark's
    box test does, and gives the parameter where the segment enters it
    */
    private static boolean segmentEntry(double[] box, int offset, Ray ray, double minimumT, double maximumT, double[] entryT) {
        double nearT = minimumT;
        double farT = maximumT;
        for ( int k = 0; k < 3; k++ ) {
            final double origin = component(ray.getOrigin(), k);
            final double direction = component(ray.getDirection(), k);
            if ( direction == 0.0 ) {
                if ( origin < box[offset + k] || origin > box[offset + 3 + k] ) {
                    return false;
                }
                continue;
            }
            double entry = (box[offset + k] - origin) / direction;
            double exit = (box[offset + 3 + k] - origin) / direction;
            if ( entry > exit ) {
                final double swap = entry;
                entry = exit;
                exit = swap;
            }
            if ( exit < nearT ) {
                return false;
            }
            if ( entry > nearT ) {
                nearT = entry;
            }
            if ( exit < farT ) {
                farT = exit;
            }
            if ( !(nearT <= farT * TOLERANCE_SCALE) ) {
                return false;
            }
        }

        if ( nearT == minimumT ) {
            if ( !(farT < maximumT) ) {
                return false;
            }
            entryT[0] = farT;
            return true;
        }
        if ( !(nearT < maximumT) ) {
            return false;
        }
        entryT[0] = nearT;
        return true;
    }

    private boolean testGroup(
        int group,
        Ray ray,
        double minimumT,
        double[] inOutBestT,
        boolean anyHit,
        PrimitiveRayTest test,
        QueryState state) {
        if ( groupBounded[group] ) {
            double[] p = new double[3];
            for ( int k = 0; k < 3; k++ ) {
                p[k] = component(ray.getOrigin(), k) + minimumT * component(ray.getDirection(), k);
            }
            double[] entryT = new double[1];
            if ( outOfBox(groupBoxes, 6 * group, p) && !segmentEntry(groupBoxes, 6 * group, ray, minimumT, inOutBestT[0], entryT) ) {
                return false;
            }
        }

        boolean accepted = false;
        for ( int i = groupFirstMember[group]; i < groupFirstMember[group + 1]; i++ ) {
            final int member = groupMembers[i];
            boolean hit;
            if ( SpatialGroupTree.isGroupMember(member) ) {
                hit = testGroup(SpatialGroupTree.memberGroup(member), ray, minimumT, inOutBestT, anyHit, test, state);
            }
            else {
                state.tests++;
                hit = test.test(member, minimumT, inOutBestT[0], state.hitT);
                if ( hit ) {
                    inOutBestT[0] = state.hitT[0];
                    state.result = new SpatialQueryResult(member, state.hitT[0]);
                }
            }
            if ( hit ) {
                accepted = true;
                if ( anyHit ) {
                    return true;
                }
            }
        }
        return accepted;
    }

    private boolean testItem(
        int item,
        Ray ray,
        double minimumT,
        double[] inOutBestT,
        boolean anyHit,
        PrimitiveRayTest test,
        SpatialIndexWorkspace workspace,
        QueryState state) {
        final int reference = itemReference[item];
        if ( itemKind[item] == PRIMITIVE_ITEM ) {
            state.tests++;
            if ( test.test(reference, minimumT, inOutBestT[0], state.hitT) ) {
                inOutBestT[0] = state.hitT[0];
                state.result = new SpatialQueryResult(reference, state.hitT[0]);
                return true;
            }
            return false;
        }
        if ( itemKind[item] == GROUP_ITEM ) {
            return testGroup(reference, ray, minimumT, inOutBestT, anyHit, test, state);
        }
        long[] visits = new long[] {0};
        final boolean hit = traverseGrid(reference, ray, minimumT, inOutBestT, anyHit, test, workspace, state, visits);
        workspace.addNodeVisits(visits[0]);
        return hit;
    }

    private boolean traverseGrid(
        int grid,
        Ray ray,
        double minimumT,
        double[] inOutBestT,
        boolean anyHit,
        PrimitiveRayTest test,
        SpatialIndexWorkspace workspace,
        QueryState state,
        long[] visits) {
        final int boxOffset = 6 * grid;
        final int resolution = gridResolution[grid];
        final int cellSizeOffset = 3 * grid;
        final boolean mailbox = workspace.getNumberOfPrimitives() >= getNumberOfItems();

        // Where the ray enters the grid
        double[] t0 = new double[] {minimumT};
        double[] p = new double[3];
        for ( int k = 0; k < 3; k++ ) {
            p[k] = component(ray.getOrigin(), k) + t0[0] * component(ray.getDirection(), k);
        }
        if ( outOfBox(gridBoxes, boxOffset, p) ) {
            if ( !segmentEntry(gridBoxes, boxOffset, ray, minimumT, inOutBestT[0], t0) ) {
                return false;
            }
            for ( int k = 0; k < 3; k++ ) {
                p[k] = component(ray.getOrigin(), k) + t0[0] * component(ray.getDirection(), k);
            }
        }

        // The cell it enters, and the parameters of the next cell boundaries
        int[] g = new int[3];
        int[] step = new int[3];
        int[] out = new int[3];
        double[] tDelta = new double[3];
        double[] tNext = new double[3];
        for ( int k = 0; k < 3; k++ ) {
            final double cellSize = gridCellSizes[cellSizeOffset + k];
            final double lo = gridBoxes[boxOffset + k];
            g[k] = cellCoordinate(p[k], lo, cellSize, resolution);
            final double direction = component(ray.getDirection(), k);
            if ( Math.abs(direction) > EPSILON ) {
                if ( direction > 0.0 ) {
                    tDelta[k] = cellSize / direction;
                    tNext[k] = t0[0] + ((g[k] + 1) * cellSize + lo - p[k]) / direction;
                    step[k] = 1;
                    out[k] = resolution;
                }
                else {
                    tDelta[k] = cellSize / -direction;
                    tNext[k] = t0[0] + (g[k] * cellSize + lo - p[k]) / direction;
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

        final int firstCell = gridFirstCell[grid];
        double t = t0[0];
        boolean accepted = false;
        boolean inGrid;
        do {
            visits[0]++;
            final int cell = firstCell + (g[0] * resolution + g[1]) * resolution + g[2];
            for ( int i = cellStart[cell]; i < cellStart[cell + 1]; i++ ) {
                final int item = cellItems[i];
                if ( !mailbox || workspace.firstVisit(item) ) {
                    if ( testItem(item, ray, minimumT, inOutBestT, anyHit, test, workspace, state) ) {
                        accepted = true;
                    }
                }
            }

            int k = 2;
            if ( tNext[0] <= tNext[1] && tNext[0] <= tNext[2] ) {
                k = 0;
            }
            else if ( tNext[1] <= tNext[2] ) {
                k = 1;
            }
            g[k] += step[k];
            t = tNext[k];
            tNext[k] += tDelta[k];
            inGrid = g[k] != out[k];
        } while ( inGrid && t <= inOutBestT[0] );

        return accepted;
    }

    @Override
    public SpatialQueryResult query(
        Ray ray,
        double minimumT,
        double maximumT,
        SpatialQueryMode mode,
        PrimitiveRayTest test,
        SpatialIndexWorkspace workspace) {
        if ( getNumberOfItems() == 0 ) {
            return new SpatialQueryResult();
        }
        if ( workspace.getNumberOfPrimitives() >= getNumberOfItems() ) {
            workspace.beginMailboxQuery();
        }
        final double[] bestT = workspace.getBestTScratch();
        bestT[0] = maximumT;
        QueryState state = new QueryState(workspace.getHitTScratch());
        long[] visits = new long[] {0};
        traverseGrid(0, ray, minimumT, bestT, mode == SpatialQueryMode.ANY_HIT, test, workspace, state, visits);
        workspace.addNodeVisits(visits[0]);
        workspace.addPrimitiveTests(state.tests);
        return state.result != null ? state.result : new SpatialQueryResult();
    }
}
