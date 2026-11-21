//= References:                                                             =
//= [WALD2007] Wald, Ingo. "On fast Construction of SAH-based Bounding      =
//=     Volume Hierarchies". Proceedings of the 2007 IEEE Symposium on      =
//=     Interactive Ray Tracing, 2007, pp. 33-40.                           =

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/spatialIndex/BinnedSahSpatialIndex.h"

/** Leaf visitor of one query: runs the caller's exact test */
class _BinnedSahQueryVisitor {
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

    BOOLEAN visit(const int primitive, double &inOutBestT)
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

/**
The loop of `BinnedSahHierarchy::traverse` of VITRAL's cpp port, on a non
empty hierarchy: a stack, nearest child first, nodes farther than the
closest hit skipped, node entry clipped at parameter zero, the traversal
ended when the visitor returns true. Node visits and leaf entries are added
to the given counters; returns false when the stack would overflow.
*/
static BOOLEAN
traverseHierarchy(
    const BinnedSahHierarchy &hierarchy,
    const Ray &ray,
    _BinnedSahQueryVisitor &visitor,
    double &inOutBestT,
    long int &nodeVisits,
    long int &entryTests)
{
    if ( hierarchy.getNodes().size() == 0 || hierarchy.getPrimitiveOrder().size() == 0 ) {
        return TRUE;
    }
    const BinnedSahHierarchyNode *nodeArray = hierarchy.getNodes().data();
    const ArrayList<int> &primitiveOrder = hierarchy.getPrimitiveOrder();
    const BinnedSahHierarchy::RaySlab slab(ray);
    const double rootDistance = slab.entry(nodeArray[0], inOutBestT);
    if ( rootDistance < 0.0 ) {
        return TRUE;
    }
    int nodeStack[BinnedSahHierarchy::STACK_SIZE];
    double distanceStack[BinnedSahHierarchy::STACK_SIZE];
    int sp = 0;
    nodeStack[sp] = 0;
    distanceStack[sp] = rootDistance;
    sp++;
    while ( sp > 0 ) {
        sp--;
        if ( distanceStack[sp] > inOutBestT ) {
            continue;
        }
        const BinnedSahHierarchyNode &node = nodeArray[nodeStack[sp]];
        nodeVisits++;
        if ( node.count > 0 ) {
            for ( int i = node.leftOrFirst; i < node.leftOrFirst + node.count; i++ ) {
                if ( visitor.visit(primitiveOrder[i], inOutBestT) ) {
                    entryTests += i - node.leftOrFirst + 1;
                    return TRUE;
                }
            }
            entryTests += node.count;
            continue;
        }
        int left = node.leftOrFirst;
        int right = left + 1;
        double dl = slab.entry(nodeArray[left], inOutBestT);
        double dr = slab.entry(nodeArray[right], inOutBestT);
        // Push the farther child first, so the nearer one is visited first.
        if ( dl >= 0.0 && dr >= 0.0 && dl < dr ) {
            const int swapNode = left;
            left = right;
            right = swapNode;
            const double swapDistance = dl;
            dl = dr;
            dr = swapDistance;
        }
        if ( sp + 2 > BinnedSahHierarchy::STACK_SIZE ) {
            return FALSE;
        }
        if ( dl >= 0.0 ) {
            nodeStack[sp] = left;
            distanceStack[sp] = dl;
            sp++;
        }
        if ( dr >= 0.0 ) {
            nodeStack[sp] = right;
            distanceStack[sp] = dr;
            sp++;
        }
    }
    return TRUE;
}

BinnedSahSpatialIndex::BinnedSahSpatialIndex(
    const ArrayList<double> &boxes,
    const ArrayList<int> &inUnboundedPrimitives,
    const int maximumLeafSize) :
    hierarchy(),
    unboundedPrimitives(),
    unbounded(),
    numberOfPrimitives(((int)(boxes.size() / 6)))
{
    long int i;
    for ( i = 0; i < numberOfPrimitives; i++ ) {
        unbounded.add(0);
    }
    for ( i = 0; i < inUnboundedPrimitives.size(); i++ ) {
        const int primitive = inUnboundedPrimitives[i];
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
    const SpatialQueryMode mode,
    PrimitiveRayTest &test,
    SpatialIndexWorkspace &workspace) const
{
    const BOOLEAN anyHit = mode == ANY_HIT;
    _BinnedSahQueryVisitor visitor(test, minimumT, anyHit);
    double bestT = maximumT;

    BOOLEAN stopped = FALSE;
    for ( long int i = 0; i < unboundedPrimitives.size() && !stopped; i++ ) {
        stopped = visitor.visit(unboundedPrimitives[i], bestT);
    }

    if ( !stopped ) {
        long int nodeVisits = 0;
        long int entryTests = 0;
        if ( !traverseHierarchy(hierarchy, ray, visitor, bestT, nodeVisits, entryTests) ) {
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
