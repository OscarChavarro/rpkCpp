//= References:                                                             =
//= [SNYD1987] Snyder, John M. and Barr, Alan H. "Ray Tracing Complex       =
//=     Models Containing Surface Tessellations". SIGGRAPH '87, Computer    =
//=     Graphics 21(4), 1987, pp. 119-128.                                  =
//=     PDF: doc/references/[SNYD1987]_*.pdf                                =

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

The cpp query steps are templates over the exact test type
(`GridSpatialIndex.txx`, `queryWith<Test>`); here they are generic methods
over `T extends PrimitiveRayTest`, with the same traversal (the JIT, not the
compiler, decides about inlining the test).
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

    // Items, dense ids: kind and the primitive, group or grid they refer to
    private final ArrayList<Integer> itemKind = new ArrayList<>();
    private final ArrayList<Integer> itemReference = new ArrayList<>();

    // Grids: padded box (6 values), cells per axis, cell size (3 values),
    // first entry of the grid in cellStart (cells + 1 entries per grid)
    private final ArrayList<Double> gridBoxes = new ArrayList<>();
    private final ArrayList<Integer> gridResolution = new ArrayList<>();
    private final ArrayList<Double> gridCellSizes = new ArrayList<>();
    private final ArrayList<Integer> gridFirstCell = new ArrayList<>();
    private final ArrayList<Integer> cellStart = new ArrayList<>();
    private final ArrayList<Integer> cellItems = new ArrayList<>();

    // Groups used as items: padded box (6 values), bounded flag, members
    // (as in SpatialGroupTree.getMember) from groupFirstMember
    private final ArrayList<Double> groupBoxes = new ArrayList<>();
    private final ArrayList<Boolean> groupBounded = new ArrayList<>();
    private final ArrayList<Integer> groupFirstMember = new ArrayList<>();
    private final ArrayList<Integer> groupMembers = new ArrayList<>();

    /**
    @param tree the groups and the primitive boxes, unpadded (the index
    pads them by DELTA_BOUND_FACTOR); its root group must have a box, and
    only the primitives that are members of a group are indexed
    */
    public GridSpatialIndex(SpatialGroupTree tree)
    {
        numberOfPrimitives = tree.getNumberOfPrimitives();
        for ( int g = 0; g < tree.getNumberOfGroups(); g++ ) {
            double[] box = new double[6];
            for ( int k = 0; k < 6; k++ ) {
                box[k] = tree.getGroupBox(g, k);
            }
            double[] padded = new double[6];
            enlargedBox(box, padded);
            for ( int k = 0; k < 6; k++ ) {
                groupBoxes.add(padded[k]);
            }
            groupBounded.add(tree.isBounded(g));
            groupFirstMember.add(groupMembers.size());
            for ( int i = 0; i < tree.getNumberOfMembers(g); i++ ) {
                groupMembers.add(tree.getMember(g, i));
            }
        }
        groupFirstMember.add(groupMembers.size());

        buildGrid(tree, 0);
    }

    private static double component(Vector3Dd v, int k)
    {
        return k == 0 ? v.x() : (k == 1 ? v.y() : v.z());
    }

    private static boolean outOfBox(double[] box, double[] p)
    {
        for ( int k = 0; k < 3; k++ ) {
            if ( p[k] < box[k] || p[k] > box[3 + k] ) {
                return true;
            }
        }
        return false;
    }

    /**
    The box enlarged on each side by DELTA_BOUND_FACTOR of its size along that
    axis, and by at least EPSILON
    */
    private static void enlargedBox(double[] box, double[] out)
    {
        for ( int k = 0; k < 3; k++ ) {
            double delta = (box[3 + k] - box[k]) * DELTA_BOUND_FACTOR;
            if ( delta < EPSILON ) {
                delta = EPSILON;
            }
            out[k] = box[k] - delta;
            out[3 + k] = box[3 + k] + delta;
        }
    }

    /** Index of the cell holding coordinate p along one axis, clamped to the grid */
    private static int cellCoordinate(double p, double lo, double size, int resolution)
    {
        int v = size < EPSILON ? 0 : (int)((p - lo) / size);
        if ( v < 0 ) {
            v = 0;
        }
        if ( v >= resolution ) {
            v = resolution - 1;
        }
        return v;
    }

    private int newItem(int kind, int reference)
    {
        itemKind.add(kind);
        itemReference.add(reference);
        return itemKind.size() - 1;
    }

    private double[] primitiveBox(SpatialGroupTree tree, int member)
    {
        double[] box = new double[6];
        for ( int k = 0; k < 6; k++ ) {
            box[k] = tree.getPrimitiveBoxes().get(6 * member + k);
        }
        return box;
    }

    /**
    Builds the grid over a group (and, recursively, its sub grids) and returns
    its id
    */
    private int buildGrid(SpatialGroupTree tree, int group)
    {
        final int grid = gridResolution.size();
        final int count = tree.getPrimitiveCount(group);
        final int resolution = (int)(Math.floor(Math.pow((double)count, 0.33333) + 1.0));

        for ( int k = 0; k < 6; k++ ) {
            gridBoxes.add(groupBoxes.get(6 * group + k));
        }
        gridResolution.add(resolution);
        for ( int k = 0; k < 3; k++ ) {
            gridCellSizes.add((gridBoxes.get(6 * grid + 3 + k) - gridBoxes.get(6 * grid + k)) / resolution);
        }
        gridFirstCell.add(0);

        final int numberOfCells = resolution * resolution * resolution;
        @SuppressWarnings("unchecked")
        ArrayList<Integer>[] cells = (ArrayList<Integer>[])new ArrayList[numberOfCells];
        for ( int c = 0; c < numberOfCells; c++ ) {
            cells[c] = null;
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
                    insertItem(grid, cells, newItem(PRIMITIVE_ITEM, member), primitiveBox(tree, member));
                }
            }
        }

        gridFirstCell.set(grid, cellStart.size());
        for ( int c = 0; c < numberOfCells; c++ ) {
            cellStart.add(cellItems.size());
            if ( cells[c] != null ) {
                for ( int i = 0; i < cells[c].size(); i++ ) {
                    cellItems.add(cells[c].get(i));
                }
                cells[c] = null;
            }
        }
        cellStart.add(cellItems.size());
        return grid;
    }

    private boolean fitsInCell(SpatialGroupTree tree, int grid, int group)
    {
        if ( !tree.isBounded(group) ) {
            return false;
        }
        for ( int k = 0; k < 3; k++ ) {
            if ( tree.getGroupBox(group, 3 + k) - tree.getGroupBox(group, k) > gridCellSizes.get(3 * grid + k) ) {
                return false;
            }
        }
        return true;
    }

    /**
    Puts a member group into a grid: as one item when it fits in a cell (a sub
    grid when it is large enough), otherwise member by member
    */
    private void insert(SpatialGroupTree tree, int grid, ArrayList<Integer>[] cells, int group)
    {
        if ( fitsInCell(tree, grid, group) ) {
            if ( tree.getPrimitiveCount(group) >= MINIMUM_ITEMS_FOR_SUB_GRID ) {
                final int subGrid = buildGrid(tree, group);
                double[] box = new double[6];
                for ( int k = 0; k < 6; k++ ) {
                    box[k] = gridBoxes.get(6 * subGrid + k);
                }
                insertItem(grid, cells, newItem(GRID_ITEM, subGrid), box);
            }
            else {
                double[] box = new double[6];
                for ( int k = 0; k < 6; k++ ) {
                    box[k] = tree.getGroupBox(group, k);
                }
                insertItem(grid, cells, newItem(GROUP_ITEM, group), box);
            }
            return;
        }

        for ( int i = 0; i < tree.getNumberOfMembers(group); i++ ) {
            final int member = tree.getMember(group, i);
            if ( SpatialGroupTree.isGroupMember(member) ) {
                insert(tree, grid, cells, SpatialGroupTree.memberGroup(member));
            }
            else {
                insertItem(grid, cells, newItem(PRIMITIVE_ITEM, member), primitiveBox(tree, member));
            }
        }
    }

    /** Lists an item in every cell its enlarged box overlaps */
    private void insertItem(int grid, ArrayList<Integer>[] cells, int item, double[] box)
    {
        double[] padded = new double[6];
        enlargedBox(box, padded);
        final int resolution = gridResolution.get(grid);
        int[] lo = new int[3];
        int[] hi = new int[3];
        for ( int k = 0; k < 3; k++ ) {
            lo[k] = cellCoordinate(padded[k], gridBoxes.get(6 * grid + k), gridCellSizes.get(3 * grid + k), resolution);
            hi[k] = cellCoordinate(padded[3 + k], gridBoxes.get(6 * grid + k), gridCellSizes.get(3 * grid + k), resolution);
        }
        for ( int a = lo[0]; a <= hi[0]; a++ ) {
            for ( int b = lo[1]; b <= hi[1]; b++ ) {
                for ( int c = lo[2]; c <= hi[2]; c++ ) {
                    final int cell = (a * resolution + b) * resolution + c;
                    if ( cells[cell] == null ) {
                        cells[cell] = new ArrayList<>();
                    }
                    cells[cell].add(item);
                }
            }
        }
    }

    @Override
    public int getNumberOfPrimitives()
    {
        return numberOfPrimitives;
    }

    @Override
    public int getWorkspaceSize()
    {
        return getNumberOfItems();
    }

    public int getNumberOfGrids()
    {
        return gridResolution.size();
    }

    public int getNumberOfItems()
    {
        return itemKind.size();
    }

    /** Cells per axis of a grid; grid 0 is the top level grid */
    public int getResolution(int grid)
    {
        return gridResolution.get(grid);
    }

    /**
    Clips the segment [minimumT, maximumT] of the ray to a box, as RenderPark's
    box test does, and gives the parameter where the segment enters it
    @param entryT one-element output holder
    */
    private boolean segmentEntry(double[] box, Ray ray, double minimumT, double maximumT, double[] entryT)
    {
        double nearT = minimumT;
        double farT = maximumT;
        for ( int k = 0; k < 3; k++ ) {
            final double origin = component(ray.getOrigin(), k);
            final double direction = component(ray.getDirection(), k);
            if ( direction == 0.0 ) {
                if ( origin < box[k] || origin > box[3 + k] ) {
                    return false;
                }
                continue;
            }
            double entry = (box[k] - origin) / direction;
            double exit = (box[3 + k] - origin) / direction;
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

    /** Six values of a box list from `6 * index` */
    private static double[] boxAt(ArrayList<Double> boxes, int index)
    {
        double[] box = new double[6];
        for ( int k = 0; k < 6; k++ ) {
            box[k] = boxes.get(6 * index + k);
        }
        return box;
    }

    /** Accumulated state of one query (cpp reference parameters) */
    private static final class QueryState {
        double bestT;
        SpatialQueryResult result = new SpatialQueryResult();
        long tests;
        final double[] hitT = new double[1];
    }

    private <T extends PrimitiveRayTest> boolean testGroup(
        int group,
        Ray ray,
        double minimumT,
        boolean anyHit,
        T test,
        QueryState state)
    {
        if ( groupBounded.get(group) ) {
            final double[] box = boxAt(groupBoxes, group);
            double[] p = new double[3];
            for ( int k = 0; k < 3; k++ ) {
                p[k] = component(ray.getOrigin(), k) + minimumT * component(ray.getDirection(), k);
            }
            double[] entryT = new double[1];
            if ( outOfBox(box, p) && !segmentEntry(box, ray, minimumT, state.bestT, entryT) ) {
                return false;
            }
        }

        boolean accepted = false;
        for ( int i = groupFirstMember.get(group); i < groupFirstMember.get(group + 1); i++ ) {
            final int member = groupMembers.get(i);
            boolean hit;
            if ( SpatialGroupTree.isGroupMember(member) ) {
                hit = testGroup(SpatialGroupTree.memberGroup(member), ray, minimumT, anyHit, test, state);
            }
            else {
                state.tests++;
                hit = test.test(member, minimumT, state.bestT, state.hitT);
                if ( hit ) {
                    state.bestT = state.hitT[0];
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

    private <T extends PrimitiveRayTest> boolean testItem(
        int item,
        Ray ray,
        double minimumT,
        boolean anyHit,
        T test,
        SpatialIndexWorkspace workspace,
        QueryState state)
    {
        final int reference = itemReference.get(item);
        if ( itemKind.get(item) == PRIMITIVE_ITEM ) {
            state.tests++;
            if ( test.test(reference, minimumT, state.bestT, state.hitT) ) {
                state.bestT = state.hitT[0];
                state.result = new SpatialQueryResult(reference, state.hitT[0]);
                return true;
            }
            return false;
        }
        if ( itemKind.get(item) == GROUP_ITEM ) {
            return testGroup(reference, ray, minimumT, anyHit, test, state);
        }
        long[] visits = {0};
        final boolean hit = traverseGrid(reference, ray, minimumT, anyHit, test, workspace, state, visits);
        workspace.addNodeVisits(visits[0]);
        return hit;
    }

    /**
    Walk of one grid, [SNYD1987] Figures 9 and 10: enter the grid, then visit
    the cells the ray crosses in order, stepping each time along the axis whose
    next cell boundary is closest.
    @param visits one-element accumulator
    */
    private <T extends PrimitiveRayTest> boolean traverseGrid(
        int grid,
        Ray ray,
        double minimumT,
        boolean anyHit,
        T test,
        SpatialIndexWorkspace workspace,
        QueryState state,
        long[] visits)
    {
        final double[] box = boxAt(gridBoxes, grid);
        final int resolution = gridResolution.get(grid);
        final double[] cellSize = new double[3];
        for ( int k = 0; k < 3; k++ ) {
            cellSize[k] = gridCellSizes.get(3 * grid + k);
        }
        final boolean mailbox = workspace.getNumberOfPrimitives() >= getNumberOfItems();

        // Where the ray enters the grid
        double t0 = minimumT;
        double[] p = new double[3];
        for ( int k = 0; k < 3; k++ ) {
            p[k] = component(ray.getOrigin(), k) + t0 * component(ray.getDirection(), k);
        }
        if ( outOfBox(box, p) ) {
            double[] entryT = {t0};
            if ( !segmentEntry(box, ray, minimumT, state.bestT, entryT) ) {
                return false;
            }
            t0 = entryT[0];
            for ( int k = 0; k < 3; k++ ) {
                p[k] = component(ray.getOrigin(), k) + t0 * component(ray.getDirection(), k);
            }
        }

        // The cell it enters, and the parameters of the next cell boundaries
        int[] g = new int[3];
        int[] step = new int[3];
        int[] out = new int[3];
        double[] tDelta = new double[3];
        double[] tNext = new double[3];
        for ( int k = 0; k < 3; k++ ) {
            g[k] = cellCoordinate(p[k], box[k], cellSize[k], resolution);
            final double direction = component(ray.getDirection(), k);
            if ( Math.abs(direction) > EPSILON ) {
                if ( direction > 0.0 ) {
                    tDelta[k] = cellSize[k] / direction;
                    tNext[k] = t0 + ((g[k] + 1) * cellSize[k] + box[k] - p[k]) / direction;
                    step[k] = 1;
                    out[k] = resolution;
                }
                else {
                    tDelta[k] = cellSize[k] / -direction;
                    tNext[k] = t0 + (g[k] * cellSize[k] + box[k] - p[k]) / direction;
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

        final int firstCell = gridFirstCell.get(grid);
        boolean accepted = false;
        boolean inGrid;
        do {
            visits[0]++;
            final int cell = firstCell + (g[0] * resolution + g[1]) * resolution + g[2];
            for ( int i = cellStart.get(cell); i < cellStart.get(cell + 1); i++ ) {
                final int item = cellItems.get(i);
                if ( !mailbox || workspace.firstVisit(item) ) {
                    if ( testItem(item, ray, minimumT, anyHit, test, workspace, state) ) {
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
            t0 = tNext[k];
            tNext[k] += tDelta[k];
            inGrid = g[k] != out[k];
        } while ( inGrid && t0 <= state.bestT );

        return accepted;
    }

    @Override
    public SpatialQueryResult query(
        Ray ray,
        double minimumT,
        double maximumT,
        SpatialQueryMode mode,
        PrimitiveRayTest test,
        SpatialIndexWorkspace workspace)
    {
        return queryWith(ray, minimumT, maximumT, mode, test, workspace);
    }

    /**
    `query` with the exact test given by its own type: the same traversal
    (cpp `GridSpatialIndex.txx`, a template so that a final test class is
    inlined); `query` is this method for `PrimitiveRayTest`.
    */
    public <T extends PrimitiveRayTest> SpatialQueryResult queryWith(
        Ray ray,
        double minimumT,
        double maximumT,
        SpatialQueryMode mode,
        T test,
        SpatialIndexWorkspace workspace)
    {
        QueryState state = new QueryState();
        if ( getNumberOfItems() == 0 ) {
            return state.result;
        }
        if ( workspace.getNumberOfPrimitives() >= getNumberOfItems() ) {
            workspace.beginMailboxQuery();
        }
        state.bestT = maximumT;
        state.tests = 0;
        long[] visits = {0};
        traverseGrid(0, ray, minimumT, mode == SpatialQueryMode.ANY_HIT, test, workspace, state, visits);
        workspace.addNodeVisits(visits[0]);
        workspace.addPrimitiveTests(state.tests);
        return state.result;
    }
}
