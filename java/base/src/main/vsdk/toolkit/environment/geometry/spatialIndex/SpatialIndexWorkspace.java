package vsdk.toolkit.environment.geometry.spatialIndex;

import vsdk.toolkit.environment.geometry.boundingVolumeHierarchy.BinnedSahHierarchy;

/**
Per-query scratch of a `SpatialIndex`: the traversal counters and, for
indexes whose cells may list one primitive several times (grids), a mailbox
that records the last query that tested each primitive.

Caller-owned and reusable: one workspace per worker, never shared between
concurrent queries. The counters accumulate over the queries until the
caller reads and clears them; nothing static is written (the counters of all
workers are merged by the caller).

Java adaptation: it also holds the traversal stack of `BinnedSahHierarchy`
and the by-reference scratch values that the C++ port keeps on the call
stack, so a query allocates no arrays.
*/
public final class SpatialIndexWorkspace {
    private long nodeVisits;
    private long primitiveTests;
    private final int[] lastQueryByPrimitive;
    private final int numberOfPrimitives;
    private int queryCount;

    private final BinnedSahHierarchy.TraversalStack traversalStack;
    private final double[] bestT;
    private final double[] hitT;
    private final long[] nodeVisitScratch;
    private final long[] entryTestScratch;

    public SpatialIndexWorkspace() {
        this(0);
    }

    /**
    @param inNumberOfPrimitives size of the mailbox: the number of primitives
    of the index, or zero when the index does not use one
    */
    public SpatialIndexWorkspace(int inNumberOfPrimitives) {
        nodeVisits = 0;
        primitiveTests = 0;
        numberOfPrimitives = inNumberOfPrimitives > 0 ? inNumberOfPrimitives : 0;
        lastQueryByPrimitive = numberOfPrimitives > 0 ? new int[numberOfPrimitives] : null;
        queryCount = 0;
        traversalStack = new BinnedSahHierarchy.TraversalStack();
        bestT = new double[1];
        hitT = new double[1];
        nodeVisitScratch = new long[1];
        entryTestScratch = new long[1];
    }

    public long getNodeVisits() {
        return nodeVisits;
    }

    public long getPrimitiveTests() {
        return primitiveTests;
    }

    public void addNodeVisits(long count) {
        nodeVisits += count;
    }

    public void addPrimitiveTests(long count) {
        primitiveTests += count;
    }

    public void clearCounters() {
        nodeVisits = 0;
        primitiveTests = 0;
    }

    public int getNumberOfPrimitives() {
        return numberOfPrimitives;
    }

    /** Starts a query that uses the mailbox */
    public void beginMailboxQuery() {
        queryCount++;
        if ( queryCount == 0 ) {
            // The counter wrapped: clear the mailbox, so that no primitive appears
            // as already reached by this query, and skip the value 0 it holds
            for ( int i = 0; i < numberOfPrimitives; i++ ) {
                lastQueryByPrimitive[i] = 0;
            }
            queryCount = 1;
        }
    }

    /**
    Records that the current query reaches a primitive; returns false when
    the query had already reached it (the primitive is then not tested
    again). Valid after beginMailboxQuery, for primitives below the mailbox
    size.
    */
    public boolean firstVisit(int primitive) {
        if ( lastQueryByPrimitive[primitive] == queryCount ) {
            return false;
        }
        lastQueryByPrimitive[primitive] = queryCount;
        return true;
    }

    BinnedSahHierarchy.TraversalStack getTraversalStack() {
        return traversalStack;
    }

    double[] getBestTScratch() {
        return bestT;
    }

    double[] getHitTScratch() {
        return hitT;
    }

    long[] getNodeVisitScratch() {
        return nodeVisitScratch;
    }

    long[] getEntryTestScratch() {
        return entryTestScratch;
    }
}
