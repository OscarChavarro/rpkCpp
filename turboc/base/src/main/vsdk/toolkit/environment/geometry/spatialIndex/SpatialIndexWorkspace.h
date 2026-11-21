#ifndef __SPATIAL_INDEX_WORKSPACE__
#define __SPATIAL_INDEX_WORKSPACE__

#include "vsdk/toolkit/common/VSDK.h"

/**
Per-query scratch of a `SpatialIndex`: the traversal counters and, for
indexes whose cells may list one primitive several times (grids), a mailbox
that records the last query that tested each primitive.

Caller-owned and reusable: one workspace per worker, never shared between
concurrent queries. The counters accumulate over the queries until the
caller reads and clears them; nothing static is written (the counters of all
workers are merged by the caller).
*/
class SpatialIndexWorkspace {
  private:
    long int nodeVisits;
    long int primitiveTests;
    unsigned int *lastQueryByPrimitive;
    int numberOfPrimitives;
    unsigned int queryCount;

    // Not copyable
    SpatialIndexWorkspace(const SpatialIndexWorkspace &);
    SpatialIndexWorkspace &operator=(const SpatialIndexWorkspace &);

  public:
    /**
    @param inNumberOfPrimitives size of the mailbox: the number of primitives
    of the index, or zero when the index does not use one
    */
    explicit SpatialIndexWorkspace(int inNumberOfPrimitives = 0);
    ~SpatialIndexWorkspace();

    long int getNodeVisits() const;
    long int getPrimitiveTests() const;
    void addNodeVisits(long int count);
    void addPrimitiveTests(long int count);
    void clearCounters();

    int getNumberOfPrimitives() const;

    /** Starts a query that uses the mailbox */
    void beginMailboxQuery();

    /**
    Records that the current query reaches a primitive; returns FALSE when
    the query had already reached it (the primitive is then not tested
    again). Valid after beginMailboxQuery, for primitives below the mailbox
    size.
    */
    BOOLEAN firstVisit(int primitive);
};

inline long int
SpatialIndexWorkspace::getNodeVisits() const
{
    return nodeVisits;
}

inline long int
SpatialIndexWorkspace::getPrimitiveTests() const
{
    return primitiveTests;
}

inline void
SpatialIndexWorkspace::addNodeVisits(const long int count)
{
    nodeVisits += count;
}

inline void
SpatialIndexWorkspace::addPrimitiveTests(const long int count)
{
    primitiveTests += count;
}

inline int
SpatialIndexWorkspace::getNumberOfPrimitives() const
{
    return numberOfPrimitives;
}

inline BOOLEAN
SpatialIndexWorkspace::firstVisit(const int primitive)
{
    if ( lastQueryByPrimitive[primitive] == queryCount ) {
        return FALSE;
    }
    lastQueryByPrimitive[primitive] = queryCount;
    return TRUE;
}

#endif
