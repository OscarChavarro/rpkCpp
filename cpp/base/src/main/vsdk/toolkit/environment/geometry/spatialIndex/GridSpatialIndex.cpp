//= References:                                                             =
//= [SNYD1987] Snyder, John M. and Barr, Alan H. "Ray Tracing Complex       =
//=     Models Containing Surface Tessellations". SIGGRAPH '87, Computer    =
//=     Graphics 21(4), 1987, pp. 119-128.                                  =
//=     PDF: doc/references/[SNYD1987]_*.pdf                                =

#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/spatialIndex/GridSpatialIndex.h"

#include "vsdk/toolkit/environment/geometry/spatialIndex/GridSpatialIndex.txx"

using namespace gridSpatialIndexDetail;

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

SpatialQueryResult
GridSpatialIndex::query(
    const Ray &ray,
    const double minimumT,
    const double maximumT,
    const SpatialQueryMode mode,
    PrimitiveRayTest &test,
    SpatialIndexWorkspace &workspace) const
{
    return queryWith(ray, minimumT, maximumT, mode, test, workspace);
}
