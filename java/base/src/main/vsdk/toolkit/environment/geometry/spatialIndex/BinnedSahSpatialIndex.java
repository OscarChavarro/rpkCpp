//= References:                                                             =
//= [WALD2007] Wald, Ingo. "On fast Construction of SAH-based Bounding      =
//=     Volume Hierarchies". Proceedings of the 2007 IEEE Symposium on      =
//=     Interactive Ray Tracing, 2007, pp. 33-40.                           =

package vsdk.toolkit.environment.geometry.spatialIndex;

import java.util.ArrayList;

import vsdk.toolkit.environment.geometry.boundingVolumeHierarchy.BinnedSahHierarchy;
import vsdk.toolkit.environment.geometry.boundingVolumeHierarchy.BinnedSahStoppingLeafVisitor;
import vsdk.toolkit.environment.geometry.element.Ray;

/**
`SpatialIndex` over a `BinnedSahHierarchy` ([WALD2007]): the binned SAH
builder and its nearest-first traversal, through
`BinnedSahHierarchy.traverse`, which can stop at the first accepted hit and
counts into the workspace instead of `RaytraceStatistics`.

Unbounded primitives are kept out of the hierarchy, on an explicit list that
every query tests first, in list order. Candidates of the hierarchy come in
traversal order, so with hits at equal parameters the accepted one depends on
the caller's tie rule; ANY_HIT stops at the first accepted candidate.
*/
public final class BinnedSahSpatialIndex extends SpatialIndex {
    private final BinnedSahHierarchy hierarchy;
    private final ArrayList<Integer> unboundedPrimitives;
    private final ArrayList<Boolean> unbounded;
    private final int numberOfPrimitives;

    /** Leaf visitor of one query: runs the caller's exact test */
    private static final class QueryVisitor implements BinnedSahStoppingLeafVisitor {
        private final PrimitiveRayTest test;
        private final double minimumT;
        private final boolean anyHit;
        private SpatialQueryResult result;
        private long tests;
        private final double[] hitT = new double[1];

        QueryVisitor(PrimitiveRayTest inTest, double inMinimumT, boolean inAnyHit)
        {
            test = inTest;
            minimumT = inMinimumT;
            anyHit = inAnyHit;
            result = new SpatialQueryResult();
            tests = 0;
        }

        @Override
        public boolean visit(int primitive, double[] inOutBestT)
        {
            tests++;
            if ( test.test(primitive, minimumT, inOutBestT[0], hitT) ) {
                inOutBestT[0] = hitT[0];
                result = new SpatialQueryResult(primitive, hitT[0]);
                return anyHit;
            }
            return false;
        }

        SpatialQueryResult getResult()
        {
            return result;
        }

        long getTests()
        {
            return tests;
        }
    }

    /**
    `BinnedSahSpatialIndex(boxes, inUnboundedPrimitives,
    BinnedSahHierarchy.DEFAULT_MAXIMUM_LEAF_SIZE)` (cpp default argument)
    */
    public BinnedSahSpatialIndex(
        ArrayList<Double> boxes,
        ArrayList<Integer> inUnboundedPrimitives)
    {
        this(boxes, inUnboundedPrimitives, BinnedSahHierarchy.DEFAULT_MAXIMUM_LEAF_SIZE);
    }

    /**
    @param boxes six values per primitive (lo x, y, z, hi x, y, z), padded by
    the caller; the box of an unbounded primitive is ignored
    @param inUnboundedPrimitives ids of the primitives without a box
    @param maximumLeafSize as in `BinnedSahHierarchy.build`
    */
    public BinnedSahSpatialIndex(
        ArrayList<Double> boxes,
        ArrayList<Integer> inUnboundedPrimitives,
        int maximumLeafSize)
    {
        hierarchy = new BinnedSahHierarchy();
        unboundedPrimitives = new ArrayList<>();
        unbounded = new ArrayList<>();
        numberOfPrimitives = boxes.size() / 6;

        for ( int i = 0; i < numberOfPrimitives; i++ ) {
            unbounded.add(false);
        }
        for ( int i = 0; i < inUnboundedPrimitives.size(); i++ ) {
            final int primitive = inUnboundedPrimitives.get(i);
            if ( primitive >= 0 && primitive < numberOfPrimitives && !unbounded.get(primitive) ) {
                unbounded.set(primitive, true);
                unboundedPrimitives.add(primitive);
            }
        }

        // An unbounded primitive gets an empty box: the hierarchy never reaches it
        ArrayList<Double> boundedBoxes = new ArrayList<>();
        for ( int i = 0; i < numberOfPrimitives; i++ ) {
            for ( int k = 0; k < 6; k++ ) {
                boundedBoxes.add(unbounded.get(i) ? (k < 3 ? 1.0 : -1.0) : boxes.get(6 * i + k));
            }
        }
        hierarchy.build(boundedBoxes, maximumLeafSize);
    }

    @Override
    public int getNumberOfPrimitives()
    {
        return numberOfPrimitives;
    }

    @Override
    public int getWorkspaceSize()
    {
        return 0;
    }

    public BinnedSahHierarchy getHierarchy()
    {
        return hierarchy;
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
        final boolean anyHit = mode == SpatialQueryMode.ANY_HIT;
        QueryVisitor visitor = new QueryVisitor(test, minimumT, anyHit);
        double[] bestT = {maximumT};

        boolean stopped = false;
        for ( int i = 0; i < unboundedPrimitives.size() && !stopped; i++ ) {
            stopped = visitor.visit(unboundedPrimitives.get(i), bestT);
        }

        if ( !stopped ) {
            long[] nodeVisits = {0};
            long[] entryTests = {0};
            if ( !hierarchy.traverse(ray, visitor, bestT, nodeVisits, entryTests) ) {
                // Stack overflow (not reachable while MAXIMUM_DEPTH < STACK_SIZE):
                // the bounded primitives are tested linearly instead
                for ( int primitive = 0; primitive < numberOfPrimitives; primitive++ ) {
                    if ( !unbounded.get(primitive) && visitor.visit(primitive, bestT) ) {
                        break;
                    }
                }
            }
            workspace.addNodeVisits(nodeVisits[0]);
        }

        workspace.addPrimitiveTests(visitor.getTests());
        return visitor.getResult();
    }
}
