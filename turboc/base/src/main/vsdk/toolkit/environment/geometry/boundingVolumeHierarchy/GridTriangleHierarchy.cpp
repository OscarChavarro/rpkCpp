//= References:                                                             =
//= [SNYD1987] Snyder, John M. and Barr, Alan H. "Ray Tracing Complex       =
//=     Models Containing Surface Tessellations". SIGGRAPH '87, Computer    =
//=     Graphics 21(4), 1987, pp. 119-128.                                  =
//=     PDF: doc/references/[SNYD1987]_*.pdf                                =

#include <stddef.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/statistics/RaytraceStatistics.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/GridTriangleHierarchy.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/GridSpatialIndex.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/PrimitiveRayTest.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialGroupTree.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialIndexWorkspace.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialQueryResult.h"

/**
The exact test of the grid query: `Triangle::intersectRay` on one triangle,
accepted when strictly closer than the hit so far, or at the same parameter
with a lower triangle index (the brute force tie rule). Keeps the face
coordinates of the accepted hit.
*/
class GridTriangleRayTest : public PrimitiveRayTest {
  private:
    const Ray &ray;
    const ArrayList<double> &positions;
    const ArrayList<int> &indexes;

  public:
    int best;
    double bestU;
    double bestV;

    GridTriangleRayTest(const Ray &inRay, const ArrayList<double> &inPositions,
        const ArrayList<int> &inIndexes) :
        ray(inRay), positions(inPositions), indexes(inIndexes), best(-1), bestU(0), bestV(0)
    {
    }

    virtual BOOLEAN test(int primitive, double minimumT, double maximumT, double *hitT)
    {
        double t, u, v;
        if ( !TriangleAccelerationStructure::testTriangle(ray, positions, indexes, primitive, t, u, v) ) {
            return FALSE;
        }
        if ( t < minimumT || t > maximumT ) {
            return FALSE;
        }
        if ( t == maximumT && !(best >= 0 && primitive < best) ) {
            return FALSE;
        }
        best = primitive;
        bestU = u;
        bestV = v;
        *hitT = t;
        return TRUE;
    }
};

GridTriangleHierarchy::GridTriangleHierarchy() : index(NULL)
{
}

GridTriangleHierarchy::GridTriangleHierarchy(const GridTriangleHierarchy &other) :
    TriangleAccelerationStructure(other), triangleBoxes(other.triangleBoxes), index(NULL)
{
    buildIndex();
}

GridTriangleHierarchy::~GridTriangleHierarchy()
{
    delete index;
}

TriangleMeshAccelerationMethod::Value
GridTriangleHierarchy::getMethod() const
{
    return TriangleMeshAccelerationMethod::VOXEL_GRID;
}

void
GridTriangleHierarchy::buildIndex()
{
    delete index;
    index = NULL;
    const int triangleCount = (int)(triangleBoxes.size() / 6);
    if ( triangleCount == 0 ) {
        return;
    }
    double rootBox[6] = {1e308, 1e308, 1e308, -1e308, -1e308, -1e308};
    int i;
    for ( i = 0; i < triangleCount; i++ ) {
        for ( int a = 0; a < 3; a++ ) {
            const double lo = triangleBoxes[6L * i + a];
            const double hi = triangleBoxes[6L * i + 3 + a];
            rootBox[a] = lo < rootBox[a] ? lo : rootBox[a];
            rootBox[a + 3] = hi > rootBox[a + 3] ? hi : rootBox[a + 3];
        }
    }
    SpatialGroupTree tree(triangleBoxes, rootBox);
    for ( i = 0; i < triangleCount; i++ ) {
        tree.addPrimitive(0, i);
    }
    index = new GridSpatialIndex(tree);
}

void
GridTriangleHierarchy::build(const ArrayList<double> &positions,
    const ArrayList<int> &indexes)
{
    recordBuiltMesh(positions, indexes);
    const int triangleCount = (int)(indexes.size() / 3);
    triangleBoxes.clear();
    for ( int i = 0; i < triangleCount; i++ ) {
        double lo[3], hi[3];
        triangleBox(positions, indexes, i, lo, hi);
        int a;
        for ( a = 0; a < 3; a++ ) {
            triangleBoxes.add(lo[a]);
        }
        for ( a = 0; a < 3; a++ ) {
            triangleBoxes.add(hi[a]);
        }
    }
    buildIndex();
}

BOOLEAN
GridTriangleHierarchy::firstHit(const Ray &ray,
    const ArrayList<double> &positions, const ArrayList<int> &indexes,
    double &outT, double &outU, double &outV, int &outTriangle) const
{
    if ( builtTriangleCount <= 0 || index == NULL ) {
        return FALSE;
    }
    GridTriangleRayTest test(ray, positions, indexes);
    // No mailbox: nothing to allocate per query (see the class comment).
    SpatialIndexWorkspace workspace(0);
    // Same initial bound as the brute force loop; Triangle::intersectRay
    // accepts only parameters above VSDK::EPSILON, so 0 loses no hit.
    // queryWith: the final test class is inlined into the cell loop (in the
    // cpp port; here it is the virtual call of `query`)
    const SpatialQueryResult result = index->queryWith(ray, 0.0, 1e308,
        SpatialQueryMode::CLOSEST_HIT, test, workspace);
    RaytraceStatistics::recordPrimitiveLevelTraversal(workspace.getNodeVisits(),
        workspace.getPrimitiveTests());
    if ( !result.hasHit() ) {
        return FALSE;
    }
    outT = result.getParameter();
    outU = test.bestU;
    outV = test.bestV;
    outTriangle = result.getPrimitive();
    return TRUE;
}

TriangleAccelerationStructure *
GridTriangleHierarchy::copy() const
{
    return new GridTriangleHierarchy(*this);
}

long int
GridTriangleHierarchy::getNodeCount() const
{
    return index == NULL ? 0 : index->getNumberOfGrids();
}

int
GridTriangleHierarchy::getDepth() const
{
    return index == NULL ? 0 : 1;
}

int
GridTriangleHierarchy::getResolution() const
{
    return index == NULL ? 0 : index->getResolution(0);
}
