//= References:                                                             =
//= [WALD2007] Wald, Ingo. "On fast Construction of SAH-based Bounding      =
//=     Volume Hierarchies". Proceedings of the 2007 IEEE Symposium on      =
//=     Interactive Ray Tracing, 2007, pp. 33-40.                           =

#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/spatialIndex/BinnedSahSpatialIndex.h"

/**
Leaf visitor of one query: runs the caller's exact test

C++ port note: a class of this file (in an unnamed namespace in the cpp
port), named after it
*/
class _BinnedSahQueryVisitor : public BinnedSahStoppingLeafVisitor {
  private:
    PrimitiveRayTest &test;
    const double minimumT;
    const BOOLEAN anyHit;
    SpatialQueryResult result;
    long int tests;

  public:
    _BinnedSahQueryVisitor(PrimitiveRayTest &inTest, const double inMinimumT, const BOOLEAN inAnyHit) :
        test(inTest), minimumT(inMinimumT), anyHit(inAnyHit), result(), tests(0)
    {
    }

    virtual BOOLEAN visit(const int primitive, double &inOutBestT)
    {
        double hitT;
        tests++;
        if ( test.test(primitive, minimumT, inOutBestT, &hitT) ) {
            inOutBestT = hitT;
            result = SpatialQueryResult(primitive, hitT);
            return anyHit;
        }
        return FALSE;
    }

    const SpatialQueryResult &getResult() const
    {
        return result;
    }

    long int getTests() const
    {
        return tests;
    }
};

BinnedSahSpatialIndex::BinnedSahSpatialIndex(
    const ArrayList<double> &boxes,
    const ArrayList<int> &inUnboundedPrimitives,
    const int maximumLeafSize) :
    hierarchy(),
    unboundedPrimitives(),
    unbounded(),
    numberOfPrimitives((int)(boxes.size() / 6))
{
    int i;
    for ( i = 0; i < numberOfPrimitives; i++ ) {
        unbounded.add(0);
    }
    for ( long int u = 0; u < inUnboundedPrimitives.size(); u++ ) {
        const int primitive = inUnboundedPrimitives[u];
        if ( primitive >= 0 && primitive < numberOfPrimitives && unbounded[primitive] == 0 ) {
            unbounded[primitive] = 1;
            unboundedPrimitives.add(primitive);
        }
    }

    // An unbounded primitive gets an empty box: the hierarchy never reaches it
    ArrayList<double> boundedBoxes;
    for ( i = 0; i < numberOfPrimitives; i++ ) {
        for ( int k = 0; k < 6; k++ ) {
            boundedBoxes.add(unbounded[i] != 0 ? (k < 3 ? 1.0 : -1.0) : boxes[6 * i + k]);
        }
    }
    hierarchy.build(boundedBoxes, maximumLeafSize);
}

BinnedSahSpatialIndex::~BinnedSahSpatialIndex()
{
}

int
BinnedSahSpatialIndex::getNumberOfPrimitives() const
{
    return numberOfPrimitives;
}

int
BinnedSahSpatialIndex::getWorkspaceSize() const
{
    return 0;
}

const BinnedSahHierarchy &
BinnedSahSpatialIndex::getHierarchy() const
{
    return hierarchy;
}

SpatialQueryResult
BinnedSahSpatialIndex::query(
    const Ray &ray,
    const double minimumT,
    const double maximumT,
    const SpatialQueryMode::Value mode,
    PrimitiveRayTest &test,
    SpatialIndexWorkspace &workspace) const
{
    const BOOLEAN anyHit = mode == SpatialQueryMode::ANY_HIT;
    _BinnedSahQueryVisitor visitor(test, minimumT, anyHit);
    double bestT = maximumT;

    BOOLEAN stopped = FALSE;
    for ( long int i = 0; i < unboundedPrimitives.size() && !stopped; i++ ) {
        stopped = visitor.visit(unboundedPrimitives[i], bestT);
    }

    if ( !stopped ) {
        long int nodeVisits = 0;
        long int entryTests = 0;
        if ( !hierarchy.traverse(ray, visitor, bestT, nodeVisits, entryTests) ) {
            // Stack overflow (not reachable while MAXIMUM_DEPTH < STACK_SIZE):
            // the bounded primitives are tested linearly instead
            for ( int primitive = 0; primitive < numberOfPrimitives; primitive++ ) {
                if ( unbounded[primitive] == 0 && visitor.visit(primitive, bestT) ) {
                    break;
                }
            }
        }
        workspace.addNodeVisits(nodeVisits);
    }

    workspace.addPrimitiveTests(visitor.getTests());
    return visitor.getResult();
}
