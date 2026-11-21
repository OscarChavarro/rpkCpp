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
    private final byte[] unbounded;
    private final int numberOfPrimitives;

    /** Leaf visitor of one query: runs the caller's exact test */
    private static final class QueryVisitor implements BinnedSahStoppingLeafVisitor {
        private final PrimitiveRayTest test;
        private final double minimumT;
        private final boolean anyHit;
        private final double[] hitT;
        private SpatialQueryResult result;
        private long tests;

        QueryVisitor(PrimitiveRayTest inTest, double inMinimumT, boolean inAnyHit, double[] inHitT) {
            test = inTest;
            minimumT = inMinimumT;
            anyHit = inAnyHit;
            hitT = inHitT;
            result = null;
            tests = 0;
        }

        @Override
        public boolean visit(int primitive, double[] inOutBestT) {
            tests++;
            if ( test.test(primitive, minimumT, inOutBestT[0], hitT) ) {
                inOutBestT[0] = hitT[0];
                result = new SpatialQueryResult(primitive, hitT[0]);
                return anyHit;
            }
            return false;
        }

        SpatialQueryResult getResult() {
            return result != null ? result : new SpatialQueryResult();
        }

        long getTests() {
            return tests;
        }
    }

    public BinnedSahSpatialIndex(ArrayList<Double> boxes, ArrayList<Integer> inUnboundedPrimitives) {
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
        int maximumLeafSize) {
        this(toArray(boxes), inUnboundedPrimitives, maximumLeafSize);
    }

    /**
    As the list constructor, with the box values in an array (Java adaptation:
    no boxing of the box values)
    */
    public BinnedSahSpatialIndex(
        double[] boxes,
        ArrayList<Integer> inUnboundedPrimitives,
        int maximumLeafSize) {
        hierarchy = new BinnedSahHierarchy();
        unboundedPrimitives = new ArrayList<>();
        numberOfPrimitives = boxes.length / 6;
        unbounded = new byte[numberOfPrimitives];
        for ( int i = 0; i < numberOfPrimitives; i++ ) {
            unbounded[i] = 0;
        }
        for ( int i = 0; i < inUnboundedPrimitives.size(); i++ ) {
            final int primitive = inUnboundedPrimitives.get(i);
            if ( primitive >= 0 && primitive < numberOfPrimitives && unbounded[primitive] == 0 ) {
                unbounded[primitive] = 1;
                unboundedPrimitives.add(primitive);
            }
        }

        // An unbounded primitive gets an empty box: the hierarchy never reaches it
        double[] boundedBoxes = new double[6 * numberOfPrimitives];
        for ( int i = 0; i < numberOfPrimitives; i++ ) {
            for ( int k = 0; k < 6; k++ ) {
                boundedBoxes[6 * i + k] = unbounded[i] != 0 ? (k < 3 ? 1.0 : -1.0) : boxes[6 * i + k];
            }
        }
        hierarchy.build(boundedBoxes, maximumLeafSize);
    }

    private static double[] toArray(ArrayList<Double> values) {
        double[] array = new double[values.size()];
        for ( int i = 0; i < array.length; i++ ) {
            array[i] = values.get(i);
        }
        return array;
    }

    @Override
    public int getNumberOfPrimitives() {
        return numberOfPrimitives;
    }

    @Override
    public int getWorkspaceSize() {
        return 0;
    }

    public BinnedSahHierarchy getHierarchy() {
        return hierarchy;
    }

    @Override
    public SpatialQueryResult query(
        Ray ray,
        double minimumT,
        double maximumT,
        SpatialQueryMode mode,
        PrimitiveRayTest test,
        SpatialIndexWorkspace workspace) {
        final boolean anyHit = mode == SpatialQueryMode.ANY_HIT;
        QueryVisitor visitor = new QueryVisitor(test, minimumT, anyHit, workspace.getHitTScratch());
        final double[] bestT = workspace.getBestTScratch();
        bestT[0] = maximumT;

        boolean stopped = false;
        for ( int i = 0; i < unboundedPrimitives.size() && !stopped; i++ ) {
            stopped = visitor.visit(unboundedPrimitives.get(i), bestT);
        }

        if ( !stopped ) {
            final long[] nodeVisits = workspace.getNodeVisitScratch();
            final long[] entryTests = workspace.getEntryTestScratch();
            nodeVisits[0] = 0;
            entryTests[0] = 0;
            if ( !hierarchy.traverse(ray, visitor, bestT, nodeVisits, entryTests, workspace.getTraversalStack()) ) {
                // Stack overflow (not reachable while MAXIMUM_DEPTH < STACK_SIZE):
                // the bounded primitives are tested linearly instead
                for ( int primitive = 0; primitive < numberOfPrimitives; primitive++ ) {
                    if ( unbounded[primitive] == 0 && visitor.visit(primitive, bestT) ) {
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
