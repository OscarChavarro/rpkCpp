//= References:                                                             =
//= [SNYD1987] Snyder, John M. and Barr, Alan H. "Ray Tracing Complex       =
//=     Models Containing Surface Tessellations". SIGGRAPH '87, Computer    =
//=     Graphics 21(4), 1987, pp. 119-128.                                  =
//=     PDF: doc/references/[SNYD1987]_*.pdf                                =

// Query of GridSpatialIndex, as templates over the type of the exact test:
// included by GridSpatialIndex.cpp (the PrimitiveRayTest instance behind
// `query`) and by callers of `queryWith` with their own final test class,
// whose `test` the compiler can then inline into the cell loop.

#ifndef __GRID_SPATIAL_INDEX_TXX__
#define __GRID_SPATIAL_INDEX_TXX__

#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/spatialIndex/GridSpatialIndex.h"

namespace gridSpatialIndexDetail {

// Smallest cell size and direction component the traversal divides by, and
// the smallest enlargement of a box side (RenderPark's EPSILON)
constexpr double EPSILON = 1e-6;

// Relative tolerance of the far end in a box/segment clip
constexpr double TOLERANCE_SCALE = 1.0 + 1e-6;

// Parameter of the next cell boundary along an axis the ray does not cross
constexpr double NO_BOUNDARY = 1e300;

inline double
component(const Vector3Dd &v, const int k)
{
    return k == 0 ? v.x() : (k == 1 ? v.y() : v.z());
}

inline bool
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


template <class Test>
bool
GridSpatialIndex::testGroup(
    const int group,
    const Ray &ray,
    const double minimumT,
    double &inOutBestT,
    const bool anyHit,
    Test &test,
    SpatialQueryResult &result,
    long int &tests) const
{
    if ( groupBounded[group] != 0 ) {
        const double *box = &groupBoxes[6 * group];
        double p[3];
        for ( int k = 0; k < 3; k++ ) {
            p[k] = gridSpatialIndexDetail::component(ray.getOrigin(), k) + minimumT * gridSpatialIndexDetail::component(ray.getDirection(), k);
        }
        double entryT;
        if ( gridSpatialIndexDetail::outOfBox(box, p) && !segmentEntry(box, ray, minimumT, inOutBestT, &entryT) ) {
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

template <class Test>
bool
GridSpatialIndex::testItem(
    const int item,
    const Ray &ray,
    const double minimumT,
    double &inOutBestT,
    const bool anyHit,
    Test &test,
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

/**
Walk of one grid, [SNYD1987] Figures 9 and 10: enter the grid, then visit
the cells the ray crosses in order, stepping each time along the axis whose
next cell boundary is closest.
*/
template <class Test>
bool
GridSpatialIndex::traverseGrid(
    const int grid,
    const Ray &ray,
    const double minimumT,
    double &inOutBestT,
    const bool anyHit,
    Test &test,
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
        p[k] = gridSpatialIndexDetail::component(ray.getOrigin(), k) + t0 * gridSpatialIndexDetail::component(ray.getDirection(), k);
    }
    if ( gridSpatialIndexDetail::outOfBox(box, p) ) {
        if ( !segmentEntry(box, ray, minimumT, inOutBestT, &t0) ) {
            return false;
        }
        for ( int k = 0; k < 3; k++ ) {
            p[k] = gridSpatialIndexDetail::component(ray.getOrigin(), k) + t0 * gridSpatialIndexDetail::component(ray.getDirection(), k);
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
        const double direction = gridSpatialIndexDetail::component(ray.getDirection(), k);
        if ( java::Math::abs(direction) > gridSpatialIndexDetail::EPSILON ) {
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
            tNext[k] = gridSpatialIndexDetail::NO_BOUNDARY;
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

template <class Test>
SpatialQueryResult
GridSpatialIndex::queryWith(
    const Ray &ray,
    const double minimumT,
    const double maximumT,
    const SpatialQueryMode mode,
    Test &test,
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

#endif
