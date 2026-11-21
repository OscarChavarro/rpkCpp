#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialIndexWorkspace.h"

SpatialIndexWorkspace::SpatialIndexWorkspace(const int inNumberOfPrimitives) :
    nodeVisits(0),
    primitiveTests(0),
    lastQueryByPrimitive(nullptr),
    numberOfPrimitives(inNumberOfPrimitives > 0 ? inNumberOfPrimitives : 0),
    queryCount(0)
{
    if ( numberOfPrimitives > 0 ) {
        lastQueryByPrimitive = new unsigned int[numberOfPrimitives];
        for ( int i = 0; i < numberOfPrimitives; i++ ) {
            lastQueryByPrimitive[i] = 0;
        }
    }
}

SpatialIndexWorkspace::~SpatialIndexWorkspace()
{
    delete[] lastQueryByPrimitive;
}

void
SpatialIndexWorkspace::clearCounters()
{
    nodeVisits = 0;
    primitiveTests = 0;
}

void
SpatialIndexWorkspace::beginMailboxQuery()
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
