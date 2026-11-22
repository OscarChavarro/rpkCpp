#include "vsdk/toolkit/environment/geometry/spatialIndex/LinearSpatialIndex.h"

LinearSpatialIndex::LinearSpatialIndex(const int inNumberOfPrimitives) :
    numberOfPrimitives(inNumberOfPrimitives > 0 ? inNumberOfPrimitives : 0)
{
}

LinearSpatialIndex::~LinearSpatialIndex()
{
}

int
LinearSpatialIndex::getNumberOfPrimitives() const
{
    return numberOfPrimitives;
}

int
LinearSpatialIndex::getWorkspaceSize() const
{
    return 0;
}

SpatialQueryResult
LinearSpatialIndex::query(
    const Ray &/*ray*/,
    const double minimumT,
    const double maximumT,
    const SpatialQueryMode::Value mode,
    PrimitiveRayTest &test,
    SpatialIndexWorkspace &workspace) const
{
    SpatialQueryResult result;
    double bestT = maximumT;
    long int tests = 0;
    for ( int primitive = 0; primitive < numberOfPrimitives; primitive++ ) {
        double hitT;
        tests++;
        if ( test.test(primitive, minimumT, bestT, &hitT) ) {
            bestT = hitT;
            result = SpatialQueryResult(primitive, hitT);
            if ( mode == SpatialQueryMode::ANY_HIT ) {
                break;
            }
        }
    }
    workspace.addPrimitiveTests(tests);
    return result;
}
