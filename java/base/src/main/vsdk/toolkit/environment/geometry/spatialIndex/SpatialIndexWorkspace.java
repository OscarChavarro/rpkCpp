package vsdk.toolkit.environment.geometry.spatialIndex;

/**
Per-query scratch of a `SpatialIndex`: the traversal counters and, for
indexes whose cells may list one primitive several times (grids), a mailbox
that records the last query that tested each primitive.

Caller-owned and reusable: one workspace per worker, never shared between
concurrent queries. The counters accumulate over the queries until the
caller reads and clears them; nothing static is written (the counters of all
workers are merged by the caller).

The query counter is an unsigned 32 bit value in cpp; here an `int` that
wraps the same way (only equality is used on it).
*/
public final class SpatialIndexWorkspace {
    private long nodeVisits;
    private long primitiveTests;
    private final int[] lastQueryByPrimitive;
    private final int numberOfPrimitives;
    private int queryCount;

    /** `SpatialIndexWorkspace(0)` (cpp default argument) */
    public SpatialIndexWorkspace()
    {
        this(0);
    }

    /**
    @param inNumberOfPrimitives size of the mailbox: the number of primitives
    of the index, or zero when the index does not use one
    */
    public SpatialIndexWorkspace(int inNumberOfPrimitives)
    {
        nodeVisits = 0;
        primitiveTests = 0;
        numberOfPrimitives = inNumberOfPrimitives > 0 ? inNumberOfPrimitives : 0;
        queryCount = 0;
        if ( numberOfPrimitives > 0 ) {
            lastQueryByPrimitive = new int[numberOfPrimitives];
            for ( int i = 0; i < numberOfPrimitives; i++ ) {
                lastQueryByPrimitive[i] = 0;
            }
        }
        else {
            lastQueryByPrimitive = null;
        }
    }

    public long getNodeVisits()
    {
        return nodeVisits;
    }

    public long getPrimitiveTests()
    {
        return primitiveTests;
    }

    public void addNodeVisits(long count)
    {
        nodeVisits += count;
    }

    public void addPrimitiveTests(long count)
    {
        primitiveTests += count;
    }

    public void clearCounters()
    {
        nodeVisits = 0;
        primitiveTests = 0;
    }

    public int getNumberOfPrimitives()
    {
        return numberOfPrimitives;
    }

    /** Starts a query that uses the mailbox */
    public void beginMailboxQuery()
    {
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
    public boolean firstVisit(int primitive)
    {
        if ( lastQueryByPrimitive[primitive] == queryCount ) {
            return false;
        }
        lastQueryByPrimitive[primitive] = queryCount;
        return true;
    }
}
