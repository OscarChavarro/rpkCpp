//= References:                                                             =
//= [SNYD1987] Snyder, John M. and Barr, Alan H. "Ray Tracing Complex       =
//=     Models Containing Surface Tessellations". SIGGRAPH '87, Computer    =
//=     Graphics 21(4), 1987, pp. 119-128.                                  =

#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/spatialIndex/GridSpatialIndex.h"

namespace {

// Smallest cell size and direction component the traversal divides by, and
// the smallest enlargement of a box side (RenderPark's EPSILON)
const double EPSILON = 1e-6;

// Relative tolerance of the far end in a box/segment clip
const double TOLERANCE_SCALE = 1.0 + 1e-6;

// Parameter of the next cell boundary along an axis the ray does not cross
const double NO_BOUNDARY = 1e300;

double
component(const Vector3Dd &v, const int k)
{
    return k == 0 ? v.x() : (k == 1 ? v.y() : v.z());
}

bool
outOfBox(const double *box, const double *p)
{
    for ( int k = 0; k < 3; k++ ) {
        if ( p[k] < box[k] || p[k] > box[3 + k] ) {
            return true;
        }
    }
    return false;
}

}

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
    for ( int g = 0; g < tree.getNumberOfGroups(); g++ ) {
        double box[6];
        for ( int k = 0; k < 6; k++ ) {
            box[k] = tree.getGroupBox(g, k);
        }
        double padded[6];
        enlargedBox(box, padded);
        for ( int k = 0; k < 6; k++ ) {
            groupBoxes.add(padded[k]);
        }
        groupBounded.add(tree.isBounded(g) ? 1 : 0);
        groupFirstMember.add(static_cast<int>(groupMembers.size()));
        for ( int i = 0; i < tree.getNumberOfMembers(g); i++ ) {
            groupMembers.add(tree.getMember(g, i));
        }
    }
    groupFirstMember.add(static_cast<int>(groupMembers.size()));

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
    int v = size < EPSILON ? 0 : static_cast<int>((p - lo) / size);
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
    return static_cast<int>(itemKind.size()) - 1;
}

/**
Builds the grid over a group (and, recursively, its sub grids) and returns
its id
*/
int
GridSpatialIndex::buildGrid(const SpatialGroupTree &tree, const int group)
{
    const int grid = static_cast<int>(gridResolution.size());
    const int count = tree.getPrimitiveCount(group);
    const int resolution = static_cast<int>(java::Math::floor(java::Math::pow(static_cast<double>(count), 0.33333) + 1.0));

    for ( int k = 0; k < 6; k++ ) {
        gridBoxes.add(groupBoxes[6 * group + k]);
    }
    gridResolution.add(resolution);
    for ( int k = 0; k < 3; k++ ) {
        gridCellSizes.add((gridBoxes[6 * grid + 3 + k] - gridBoxes[6 * grid + k]) / resolution);
    }
    gridFirstCell.add(0);

    const int numberOfCells = resolution * resolution * resolution;
    java::ArrayList<int> **cells = new java::ArrayList<int> *[numberOfCells];
    for ( int c = 0; c < numberOfCells; c++ ) {
        cells[c] = nullptr;
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
                for ( int k = 0; k < 6; k++ ) {
                    box[k] = tree.getPrimitiveBoxes()[6 * member + k];
                }
                insertItem(grid, cells, newItem(PRIMITIVE_ITEM, member), box);
            }
        }
    }

    gridFirstCell[grid] = static_cast<int>(cellStart.size());
    for ( int c = 0; c < numberOfCells; c++ ) {
        cellStart.add(static_cast<int>(cellItems.size()));
        if ( cells[c] != nullptr ) {
            for ( long int i = 0; i < cells[c]->size(); i++ ) {
                cellItems.add((*cells[c])[i]);
            }
            delete cells[c];
        }
    }
    cellStart.add(static_cast<int>(cellItems.size()));
    delete[] cells;
    return grid;
}

bool
GridSpatialIndex::fitsInCell(const SpatialGroupTree &tree, const int grid, const int group) const
{
    if ( !tree.isBounded(group) ) {
        return false;
    }
    for ( int k = 0; k < 3; k++ ) {
        if ( tree.getGroupBox(group, 3 + k) - tree.getGroupBox(group, k) > gridCellSizes[3 * grid + k] ) {
            return false;
        }
    }
    return true;
}

/**
Puts a member group into a grid: as one item when it fits in a cell (a sub
grid when it is large enough), otherwise member by member
*/
void
GridSpatialIndex::insert(const SpatialGroupTree &tree, const int grid, java::ArrayList<int> **cells, const int group)
{
    if ( fitsInCell(tree, grid, group) ) {
        if ( tree.getPrimitiveCount(group) >= MINIMUM_ITEMS_FOR_SUB_GRID ) {
            const int subGrid = buildGrid(tree, group);
            double box[6];
            for ( int k = 0; k < 6; k++ ) {
                box[k] = gridBoxes[6 * subGrid + k];
            }
            insertItem(grid, cells, newItem(GRID_ITEM, subGrid), box);
        } else {
            double box[6];
            for ( int k = 0; k < 6; k++ ) {
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
            double box[6];
            for ( int k = 0; k < 6; k++ ) {
                box[k] = tree.getPrimitiveBoxes()[6 * member + k];
            }
            insertItem(grid, cells, newItem(PRIMITIVE_ITEM, member), box);
        }
    }
}

/** Lists an item in every cell its enlarged box overlaps */
void
GridSpatialIndex::insertItem(const int grid, java::ArrayList<int> **cells, const int item, const double *box) const
{
    double padded[6];
    enlargedBox(box, padded);
    const int resolution = gridResolution[grid];
    int lo[3];
    int hi[3];
    for ( int k = 0; k < 3; k++ ) {
        lo[k] = cellCoordinate(padded[k], gridBoxes[6 * grid + k], gridCellSizes[3 * grid + k], resolution);
        hi[k] = cellCoordinate(padded[3 + k], gridBoxes[6 * grid + k], gridCellSizes[3 * grid + k], resolution);
    }
    for ( int a = lo[0]; a <= hi[0]; a++ ) {
        for ( int b = lo[1]; b <= hi[1]; b++ ) {
            for ( int c = lo[2]; c <= hi[2]; c++ ) {
                const int cell = (a * resolution + b) * resolution + c;
                if ( cells[cell] == nullptr ) {
                    cells[cell] = new java::ArrayList<int>();
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
    return static_cast<int>(gridResolution.size());
}

int
GridSpatialIndex::getNumberOfItems() const
{
    return static_cast<int>(itemKind.size());
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
bool
GridSpatialIndex::segmentEntry(const double *box, const Ray &ray, const double minimumT, const double maximumT, double *entryT) const
{
    double nearT = minimumT;
    double farT = maximumT;
    for ( int k = 0; k < 3; k++ ) {
        const double origin = component(ray.getOrigin(), k);
        const double direction = component(ray.getDirection(), k);
        if ( direction == 0.0 ) {
            if ( origin < box[k] || origin > box[3 + k] ) {
                return false;
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
        *entryT = farT;
        return true;
    }
    if ( !(nearT < maximumT) ) {
        return false;
    }
    *entryT = nearT;
    return true;
}

bool
GridSpatialIndex::testGroup(
    const int group,
    const Ray &ray,
    const double minimumT,
    double &inOutBestT,
    const bool anyHit,
    PrimitiveRayTest &test,
    SpatialQueryResult &result,
    long int &tests) const
{
    if ( groupBounded[group] != 0 ) {
        const double *box = &groupBoxes[6 * group];
        double p[3];
        for ( int k = 0; k < 3; k++ ) {
            p[k] = component(ray.getOrigin(), k) + minimumT * component(ray.getDirection(), k);
        }
        double entryT;
        if ( outOfBox(box, p) && !segmentEntry(box, ray, minimumT, inOutBestT, &entryT) ) {
            return false;
        }
    }

    bool accepted = false;
    for ( int i = groupFirstMember[group]; i < groupFirstMember[group + 1]; i++ ) {
        const int member = groupMembers[i];
        bool hit;
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
            accepted = true;
            if ( anyHit ) {
                return true;
            }
        }
    }
    return accepted;
}

bool
GridSpatialIndex::testItem(
    const int item,
    const Ray &ray,
    const double minimumT,
    double &inOutBestT,
    const bool anyHit,
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
            return true;
        }
        return false;
    }
    if ( itemKind[item] == GROUP_ITEM ) {
        return testGroup(reference, ray, minimumT, inOutBestT, anyHit, test, result, tests);
    }
    long int visits = 0;
    const bool hit = traverseGrid(reference, ray, minimumT, inOutBestT, anyHit, test, workspace, result, tests, visits);
    workspace.addNodeVisits(visits);
    return hit;
}

bool
GridSpatialIndex::traverseGrid(
    const int grid,
    const Ray &ray,
    const double minimumT,
    double &inOutBestT,
    const bool anyHit,
    PrimitiveRayTest &test,
    SpatialIndexWorkspace &workspace,
    SpatialQueryResult &result,
    long int &tests,
    long int &visits) const
{
    const double *box = &gridBoxes[6 * grid];
    const int resolution = gridResolution[grid];
    const double *cellSize = &gridCellSizes[3 * grid];
    const bool mailbox = workspace.getNumberOfPrimitives() >= getNumberOfItems();

    // Where the ray enters the grid
    double t0 = minimumT;
    double p[3];
    for ( int k = 0; k < 3; k++ ) {
        p[k] = component(ray.getOrigin(), k) + t0 * component(ray.getDirection(), k);
    }
    if ( outOfBox(box, p) ) {
        if ( !segmentEntry(box, ray, minimumT, inOutBestT, &t0) ) {
            return false;
        }
        for ( int k = 0; k < 3; k++ ) {
            p[k] = component(ray.getOrigin(), k) + t0 * component(ray.getDirection(), k);
        }
    }

    // The cell it enters, and the parameters of the next cell boundaries
    int g[3];
    int step[3];
    int out[3];
    double tDelta[3];
    double tNext[3];
    for ( int k = 0; k < 3; k++ ) {
        g[k] = cellCoordinate(p[k], box[k], cellSize[k], resolution);
        const double direction = component(ray.getDirection(), k);
        if ( java::Math::abs(direction) > EPSILON ) {
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
    bool accepted = false;
    bool inGrid;
    do {
        visits++;
        const int cell = firstCell + (g[0] * resolution + g[1]) * resolution + g[2];
        for ( int i = cellStart[cell]; i < cellStart[cell + 1]; i++ ) {
            const int item = cellItems[i];
            if ( !mailbox || workspace.firstVisit(item) ) {
                if ( testItem(item, ray, minimumT, inOutBestT, anyHit, test, workspace, result, tests) ) {
                    accepted = true;
                }
            }
        }

        int k = 2;
        if ( tNext[0] <= tNext[1] && tNext[0] <= tNext[2] ) {
            k = 0;
        } else if ( tNext[1] <= tNext[2] ) {
            k = 1;
        }
        g[k] += step[k];
        t0 = tNext[k];
        tNext[k] += tDelta[k];
        inGrid = g[k] != out[k];
    } while ( inGrid && t0 <= inOutBestT );

    return accepted;
}

SpatialQueryResult
GridSpatialIndex::query(
    const Ray &ray,
    const double minimumT,
    const double maximumT,
    const SpatialQueryMode mode,
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
