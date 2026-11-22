//= References:                                                             =
//= [SNYD1987] Snyder, John M. and Barr, Alan H. "Ray Tracing Complex       =
//=     Models Containing Surface Tessellations". SIGGRAPH '87, Computer    =
//=     Graphics 21(4), 1987, pp. 119-128.                                  =
//=     PDF: doc/references/[SNYD1987]_*.pdf                                =

#ifndef __GRID_SPATIAL_INDEX__
#define __GRID_SPATIAL_INDEX__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialGroupTree.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialIndex.h"

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

C++ port note: the query steps of the cpp port are member templates over
the type of the exact test (its GridSpatialIndex.txx), which the compilers
of this port do not have: here they take a `PrimitiveRayTest` (one virtual
call per candidate), and `queryWith` is `query`.
*/
class GridSpatialIndex : public SpatialIndex {
  public:
    enum { MINIMUM_ITEMS_FOR_SUB_GRID = 10 };
    static const double DELTA_BOUND_FACTOR;

  private:
    enum {
        PRIMITIVE_ITEM = 0,
        GROUP_ITEM = 1,
        GRID_ITEM = 2
    };

    int numberOfPrimitives;

    // Items, dense ids: kind and the primitive, group or grid they refer to
    ArrayList<int> itemKind;
    ArrayList<int> itemReference;

    // Grids: padded box (6 values), cells per axis, cell size (3 values),
    // first entry of the grid in cellStart (cells + 1 entries per grid)
    ArrayList<double> gridBoxes;
    ArrayList<int> gridResolution;
    ArrayList<double> gridCellSizes;
    ArrayList<int> gridFirstCell;
    ArrayList<int> cellStart;
    ArrayList<int> cellItems;

    // Groups used as items: padded box (6 values), bounded flag, members
    // (as in SpatialGroupTree::getMember) from groupFirstMember
    ArrayList<double> groupBoxes;
    ArrayList<char> groupBounded;
    ArrayList<int> groupFirstMember;
    ArrayList<int> groupMembers;

    static void enlargedBox(const double *box, double *out);
    static int cellCoordinate(double p, double lo, double size, int resolution);

    int newItem(int kind, int reference);
    int buildGrid(const SpatialGroupTree &tree, int group);
    void insert(const SpatialGroupTree &tree, int grid, ArrayList<int> **cells, int group);
    void insertItem(int grid, ArrayList<int> **cells, int item, const double *box) const;
    BOOLEAN fitsInCell(const SpatialGroupTree &tree, int grid, int group) const;

    BOOLEAN segmentEntry(const double *box, const Ray &ray, double minimumT, double maximumT, double *entryT) const;
    // Query steps (templates over the exact test in the cpp port)
    BOOLEAN testItem(int item, const Ray &ray, double minimumT, double &inOutBestT, BOOLEAN anyHit,
        PrimitiveRayTest &test, SpatialIndexWorkspace &workspace, SpatialQueryResult &result, long int &tests) const;
    BOOLEAN testGroup(int group, const Ray &ray, double minimumT, double &inOutBestT, BOOLEAN anyHit,
        PrimitiveRayTest &test, SpatialQueryResult &result, long int &tests) const;
    BOOLEAN traverseGrid(int grid, const Ray &ray, double minimumT, double &inOutBestT, BOOLEAN anyHit,
        PrimitiveRayTest &test, SpatialIndexWorkspace &workspace, SpatialQueryResult &result,
        long int &tests, long int &visits) const;

  public:
    /**
    @param tree the groups and the primitive boxes, unpadded (the index
    pads them by DELTA_BOUND_FACTOR); its root group must have a box, and
    only the primitives that are members of a group are indexed
    */
    explicit GridSpatialIndex(const SpatialGroupTree &tree);
    virtual ~GridSpatialIndex();

    virtual int getNumberOfPrimitives() const;
    virtual int getWorkspaceSize() const;
    int getNumberOfGrids() const;
    int getNumberOfItems() const;

    /** Cells per axis of a grid; grid 0 is the top level grid */
    int getResolution(int grid) const;

    virtual SpatialQueryResult query(
        const Ray &ray,
        double minimumT,
        double maximumT,
        SpatialQueryMode::Value mode,
        PrimitiveRayTest &test,
        SpatialIndexWorkspace &workspace) const;

    /**
    `query` with the exact test given by its own type in the cpp port (a
    member template there, so that the compiler can inline a final test
    class into the cell loop); here the same traversal as `query`.
    */
    SpatialQueryResult queryWith(
        const Ray &ray,
        double minimumT,
        double maximumT,
        SpatialQueryMode::Value mode,
        PrimitiveRayTest &test,
        SpatialIndexWorkspace &workspace) const;
};

#endif
