//= References:                                                             =
//= [SNYD1987] Snyder, John M. and Barr, Alan H. "Ray Tracing Complex       =
//=     Models Containing Surface Tessellations". SIGGRAPH '87, Computer    =
//=     Graphics 21(4), 1987, pp. 119-128.                                  =
//=     PDF: doc/references/[SNYD1987]_*.pdf                                =

#include <stddef.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/spatialIndex/GridSpatialIndex.h"

// C++ port note: the query of the cpp port is in GridSpatialIndex.txx, as
// templates over the type of the exact test; here it is below, over
// PrimitiveRayTest, and the helpers of its gridSpatialIndexDetail namespace
// are statics of this file.

// Smallest cell size and direction component the traversal divides by, and
// the smallest enlargement of a box side (RenderPark's EPSILON)
static const double EPSILON = 1e-6;

// Relative tolerance of the far end in a box/segment clip
static const double TOLERANCE_SCALE = 1.0 + 1e-6;

// Parameter of the next cell boundary along an axis the ray does not cross
static const double NO_BOUNDARY = 1e300;

static double
component(const Vector3Dd &v, const int k)
{
    return k == 0 ? v.x() : (k == 1 ? v.y() : v.z());
}

static BOOLEAN
outOfBox(const double *box, const double *p)
{
    for ( int k = 0; k < 3; k++ ) {
        if ( p[k] < box[k] || p[k] > box[3 + k] ) {
            return TRUE;
        }
    }
    return FALSE;
}

const double GridSpatialIndex::DELTA_BOUND_FACTOR = 1e-4;

GridSpatialIndex::GridSpatialIndex(const SpatialGroupTree &tree) :
    numberOfPrimitives(tree.getNumberOfPrimitives()),
    itemKind(),
    itemReference(),
    gridBoxes(),
    gridResolution(),
    gridCellSizes(),
    gridFirstCell(),
    cellStart(),
    cellItems(),
    groupBoxes(),
    groupBounded(),
    groupFirstMember(),
    groupMembers()
{
    int k;
    for ( int g = 0; g < tree.getNumberOfGroups(); g++ ) {
        double box[6];
        for ( k = 0; k < 6; k++ ) {
            box[k] = tree.getGroupBox(g, k);
        }
        double padded[6];
        enlargedBox(box, padded);
        for ( k = 0; k < 6; k++ ) {
            groupBoxes.add(padded[k]);
        }
        groupBounded.add(tree.isBounded(g) ? 1 : 0);
        groupFirstMember.add((int)groupMembers.size());
        for ( int i = 0; i < tree.getNumberOfMembers(g); i++ ) {
            groupMembers.add(tree.getMember(g, i));
        }
    }
    groupFirstMember.add((int)groupMembers.size());

    buildGrid(tree, 0);
}

GridSpatialIndex::~GridSpatialIndex()
{
}

/**
The box enlarged on each side by DELTA_BOUND_FACTOR of its size along that
axis, and by at least EPSILON
*/
void
GridSpatialIndex::enlargedBox(const double *box, double *out)
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
int
GridSpatialIndex::cellCoordinate(const double p, const double lo, const double size, const int resolution)
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

int
GridSpatialIndex::newItem(const int kind, const int reference)
{
    itemKind.add(kind);
    itemReference.add(reference);
    return (int)itemKind.size() - 1;
}

/**
Builds the grid over a group (and, recursively, its sub grids) and returns
its id
*/
int
GridSpatialIndex::buildGrid(const SpatialGroupTree &tree, const int group)
{
    const int grid = (int)gridResolution.size();
    const int count = tree.getPrimitiveCount(group);
    const int resolution = (int)(Math::floor(Math::pow((double)count, 0.33333) + 1.0));
    int k;
    int c;

    for ( k = 0; k < 6; k++ ) {
        gridBoxes.add(groupBoxes[6L * group + k]);
    }
    gridResolution.add(resolution);
    for ( k = 0; k < 3; k++ ) {
        gridCellSizes.add((gridBoxes[6L * grid + 3 + k] - gridBoxes[6L * grid + k]) / resolution);
    }
    gridFirstCell.add(0);

    const int numberOfCells = resolution * resolution * resolution;
    ArrayList<int> **cells = new ArrayList<int> *[numberOfCells];
    for ( c = 0; c < numberOfCells; c++ ) {
        cells[c] = NULL;
    }

    if ( resolution == 1 ) {
        insert(tree, grid, cells, group);
    } else {
        // The group of a grid never fits in one of its cells (unless its box
        // is degenerate, where this avoids an endless recursion)
        for ( int i = 0; i < tree.getNumberOfMembers(group); i++ ) {
            const int member = tree.getMember(group, i);
            if ( SpatialGroupTree::isGroupMember(member) ) {
                insert(tree, grid, cells, SpatialGroupTree::memberGroup(member));
            } else {
                double box[6];
                for ( k = 0; k < 6; k++ ) {
                    box[k] = tree.getPrimitiveBoxes()[6L * member + k];
                }
                insertItem(grid, cells, newItem(PRIMITIVE_ITEM, member), box);
            }
        }
    }

    gridFirstCell[grid] = (int)cellStart.size();
    for ( c = 0; c < numberOfCells; c++ ) {
        cellStart.add((int)cellItems.size());
        if ( cells[c] != NULL ) {
            for ( long int i = 0; i < cells[c]->size(); i++ ) {
                cellItems.add((*cells[c])[i]);
            }
            delete cells[c];
        }
    }
    cellStart.add((int)cellItems.size());
    delete[] cells;
    return grid;
}

BOOLEAN
GridSpatialIndex::fitsInCell(const SpatialGroupTree &tree, const int grid, const int group) const
{
    if ( !tree.isBounded(group) ) {
        return FALSE;
    }
    for ( int k = 0; k < 3; k++ ) {
        if ( tree.getGroupBox(group, 3 + k) - tree.getGroupBox(group, k) > gridCellSizes[3L * grid + k] ) {
            return FALSE;
        }
    }
    return TRUE;
}

/**
Puts a member group into a grid: as one item when it fits in a cell (a sub
grid when it is large enough), otherwise member by member
*/
void
GridSpatialIndex::insert(const SpatialGroupTree &tree, const int grid, ArrayList<int> **cells, const int group)
{
    double box[6];
    int k;
    if ( fitsInCell(tree, grid, group) ) {
        if ( tree.getPrimitiveCount(group) >= MINIMUM_ITEMS_FOR_SUB_GRID ) {
            const int subGrid = buildGrid(tree, group);
            for ( k = 0; k < 6; k++ ) {
                box[k] = gridBoxes[6L * subGrid + k];
            }
            insertItem(grid, cells, newItem(GRID_ITEM, subGrid), box);
        } else {
            for ( k = 0; k < 6; k++ ) {
                box[k] = tree.getGroupBox(group, k);
            }
            insertItem(grid, cells, newItem(GROUP_ITEM, group), box);
        }
        return;
    }

    for ( int i = 0; i < tree.getNumberOfMembers(group); i++ ) {
        const int member = tree.getMember(group, i);
        if ( SpatialGroupTree::isGroupMember(member) ) {
            insert(tree, grid, cells, SpatialGroupTree::memberGroup(member));
        } else {
            for ( k = 0; k < 6; k++ ) {
                box[k] = tree.getPrimitiveBoxes()[6L * member + k];
            }
            insertItem(grid, cells, newItem(PRIMITIVE_ITEM, member), box);
        }
    }
}

/** Lists an item in every cell its enlarged box overlaps */
void
GridSpatialIndex::insertItem(const int grid, ArrayList<int> **cells, const int item, const double *box) const
{
    double padded[6];
    enlargedBox(box, padded);
    const int resolution = gridResolution[grid];
    int lo[3];
    int hi[3];
    for ( int k = 0; k < 3; k++ ) {
        lo[k] = cellCoordinate(padded[k], gridBoxes[6L * grid + k], gridCellSizes[3L * grid + k], resolution);
        hi[k] = cellCoordinate(padded[3 + k], gridBoxes[6L * grid + k], gridCellSizes[3L * grid + k], resolution);
    }
    for ( int a = lo[0]; a <= hi[0]; a++ ) {
        for ( int b = lo[1]; b <= hi[1]; b++ ) {
            for ( int c = lo[2]; c <= hi[2]; c++ ) {
                const int cell = (a * resolution + b) * resolution + c;
                if ( cells[cell] == NULL ) {
                    cells[cell] = new ArrayList<int>();
                }
                cells[cell]->add(item);
            }
        }
    }
}

int
GridSpatialIndex::getNumberOfPrimitives() const
{
    return numberOfPrimitives;
}

int
GridSpatialIndex::getWorkspaceSize() const
{
    return getNumberOfItems();
}

int
GridSpatialIndex::getNumberOfGrids() const
{
    return (int)gridResolution.size();
}

int
GridSpatialIndex::getNumberOfItems() const
{
    return (int)itemKind.size();
}

int
GridSpatialIndex::getResolution(const int grid) const
{
    return gridResolution[grid];
}

/**
Clips the segment [minimumT, maximumT] of the ray to a box, as RenderPark's
box test does, and gives the parameter where the segment enters it
*/
BOOLEAN
GridSpatialIndex::segmentEntry(const double *box, const Ray &ray, const double minimumT, const double maximumT, double *entryT) const
{
    double nearT = minimumT;
    double farT = maximumT;
    for ( int k = 0; k < 3; k++ ) {
        const double origin = component(ray.getOrigin(), k);
        const double direction = component(ray.getDirection(), k);
        if ( direction == 0.0 ) {
            if ( origin < box[k] || origin > box[3 + k] ) {
                return FALSE;
            }
            continue;
        }
        double entry = (box[k] - origin) / direction;
        double exit = (box[3 + k] - origin) / direction;
        if ( entry > exit ) {
            const double swap = entry;
            entry = exit;
            exit = swap;
        }
        if ( exit < nearT ) {
            return FALSE;
        }
        if ( entry > nearT ) {
            nearT = entry;
        }
        if ( exit < farT ) {
            farT = exit;
        }
        if ( !(nearT <= farT * TOLERANCE_SCALE) ) {
            return FALSE;
        }
    }

    if ( nearT == minimumT ) {
        if ( !(farT < maximumT) ) {
            return FALSE;
        }
        *entryT = farT;
        return TRUE;
    }
    if ( !(nearT < maximumT) ) {
        return FALSE;
    }
    *entryT = nearT;
    return TRUE;
}

BOOLEAN
GridSpatialIndex::testGroup(
    const int group,
    const Ray &ray,
    const double minimumT,
    double &inOutBestT,
    const BOOLEAN anyHit,
    PrimitiveRayTest &test,
    SpatialQueryResult &result,
    long int &tests) const
{
    if ( groupBounded[group] != 0 ) {
        const double *box = &groupBoxes[6L * group];
        double p[3];
        for ( int k = 0; k < 3; k++ ) {
            p[k] = component(ray.getOrigin(), k) + minimumT * component(ray.getDirection(), k);
        }
        double entryT;
        if ( outOfBox(box, p) && !segmentEntry(box, ray, minimumT, inOutBestT, &entryT) ) {
            return FALSE;
        }
    }

    BOOLEAN accepted = FALSE;
    for ( int i = groupFirstMember[group]; i < groupFirstMember[group + 1]; i++ ) {
        const int member = groupMembers[i];
        BOOLEAN hit;
        if ( SpatialGroupTree::isGroupMember(member) ) {
            hit = testGroup(SpatialGroupTree::memberGroup(member), ray, minimumT, inOutBestT, anyHit, test, result, tests);
        } else {
            double t;
            tests++;
            hit = test.test(member, minimumT, inOutBestT, &t);
            if ( hit ) {
                inOutBestT = t;
                result = SpatialQueryResult(member, t);
            }
        }
        if ( hit ) {
            accepted = TRUE;
            if ( anyHit ) {
                return TRUE;
            }
        }
    }
    return accepted;
}

BOOLEAN
GridSpatialIndex::testItem(
    const int item,
    const Ray &ray,
    const double minimumT,
    double &inOutBestT,
    const BOOLEAN anyHit,
    PrimitiveRayTest &test,
    SpatialIndexWorkspace &workspace,
    SpatialQueryResult &result,
    long int &tests) const
{
    const int reference = itemReference[item];
    if ( itemKind[item] == PRIMITIVE_ITEM ) {
        double t;
        tests++;
        if ( test.test(reference, minimumT, inOutBestT, &t) ) {
            inOutBestT = t;
            result = SpatialQueryResult(reference, t);
            return TRUE;
        }
        return FALSE;
    }
    if ( itemKind[item] == GROUP_ITEM ) {
        return testGroup(reference, ray, minimumT, inOutBestT, anyHit, test, result, tests);
    }
    long int visits = 0;
    const BOOLEAN hit = traverseGrid(reference, ray, minimumT, inOutBestT, anyHit, test, workspace, result, tests, visits);
    workspace.addNodeVisits(visits);
    return hit;
}

/**
Walk of one grid, [SNYD1987] Figures 9 and 10: enter the grid, then visit
the cells the ray crosses in order, stepping each time along the axis whose
next cell boundary is closest.
*/
BOOLEAN
GridSpatialIndex::traverseGrid(
    const int grid,
    const Ray &ray,
    const double minimumT,
    double &inOutBestT,
    const BOOLEAN anyHit,
    PrimitiveRayTest &test,
    SpatialIndexWorkspace &workspace,
    SpatialQueryResult &result,
    long int &tests,
    long int &visits) const
{
    const double *box = &gridBoxes[6L * grid];
    const int resolution = gridResolution[grid];
    const double *cellSize = &gridCellSizes[3L * grid];
    const BOOLEAN mailbox = workspace.getNumberOfPrimitives() >= getNumberOfItems();
    int k;

    // Where the ray enters the grid
    double t0 = minimumT;
    double p[3];
    for ( k = 0; k < 3; k++ ) {
        p[k] = component(ray.getOrigin(), k) + t0 * component(ray.getDirection(), k);
    }
    if ( outOfBox(box, p) ) {
        if ( !segmentEntry(box, ray, minimumT, inOutBestT, &t0) ) {
            return FALSE;
        }
        for ( k = 0; k < 3; k++ ) {
            p[k] = component(ray.getOrigin(), k) + t0 * component(ray.getDirection(), k);
        }
    }

    // The cell it enters, and the parameters of the next cell boundaries
    int g[3];
    int step[3];
    int out[3];
    double tDelta[3];
    double tNext[3];
    for ( k = 0; k < 3; k++ ) {
        g[k] = cellCoordinate(p[k], box[k], cellSize[k], resolution);
        const double direction = component(ray.getDirection(), k);
        if ( Math::abs(direction) > EPSILON ) {
            if ( direction > 0.0 ) {
                tDelta[k] = cellSize[k] / direction;
                tNext[k] = t0 + ((g[k] + 1) * cellSize[k] + box[k] - p[k]) / direction;
                step[k] = 1;
                out[k] = resolution;
            } else {
                tDelta[k] = cellSize[k] / -direction;
                tNext[k] = t0 + (g[k] * cellSize[k] + box[k] - p[k]) / direction;
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

    const int firstCell = gridFirstCell[grid];
    BOOLEAN accepted = FALSE;
    BOOLEAN inGrid;
    do {
        visits++;
        const int cell = firstCell + (g[0] * resolution + g[1]) * resolution + g[2];
        for ( int i = cellStart[cell]; i < cellStart[cell + 1]; i++ ) {
            const int item = cellItems[i];
            if ( !mailbox || workspace.firstVisit(item) ) {
                if ( testItem(item, ray, minimumT, inOutBestT, anyHit, test, workspace, result, tests) ) {
                    accepted = TRUE;
                }
            }
        }

        int axis = 2;
        if ( tNext[0] <= tNext[1] && tNext[0] <= tNext[2] ) {
            axis = 0;
        } else if ( tNext[1] <= tNext[2] ) {
            axis = 1;
        }
        g[axis] += step[axis];
        t0 = tNext[axis];
        tNext[axis] += tDelta[axis];
        inGrid = g[axis] != out[axis];
    } while ( inGrid && t0 <= inOutBestT );

    return accepted;
}

SpatialQueryResult
GridSpatialIndex::queryWith(
    const Ray &ray,
    const double minimumT,
    const double maximumT,
    const SpatialQueryMode::Value mode,
    PrimitiveRayTest &test,
    SpatialIndexWorkspace &workspace) const
{
    SpatialQueryResult result;
    if ( getNumberOfItems() == 0 ) {
        return result;
    }
    if ( workspace.getNumberOfPrimitives() >= getNumberOfItems() ) {
        workspace.beginMailboxQuery();
    }
    double bestT = maximumT;
    long int tests = 0;
    long int visits = 0;
    traverseGrid(0, ray, minimumT, bestT, mode == SpatialQueryMode::ANY_HIT, test, workspace, result, tests, visits);
    workspace.addNodeVisits(visits);
    workspace.addPrimitiveTests(tests);
    return result;
}

SpatialQueryResult
GridSpatialIndex::query(
    const Ray &ray,
    const double minimumT,
    const double maximumT,
    const SpatialQueryMode::Value mode,
    PrimitiveRayTest &test,
    SpatialIndexWorkspace &workspace) const
{
    return queryWith(ray, minimumT, maximumT, mode, test, workspace);
}
