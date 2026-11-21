#ifndef __LINEAR_SPATIAL_INDEX__
#define __LINEAR_SPATIAL_INDEX__

#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialIndex.h"

/**
Reference `SpatialIndex`: no pruning at all. Every primitive is tested, in
increasing id order, so its answer is the one every conservative index must
agree with on visibility; with a tie rule that prefers the lowest id it is
also the closest-hit answer of brute force.
*/
class LinearSpatialIndex final : public SpatialIndex {
  private:
    int numberOfPrimitives;

  public:
    explicit LinearSpatialIndex(int inNumberOfPrimitives);
    ~LinearSpatialIndex() final;

    int getNumberOfPrimitives() const final;
    int getWorkspaceSize() const final;
    SpatialQueryResult query(
        const Ray &ray,
        double minimumT,
        double maximumT,
        SpatialQueryMode mode,
        PrimitiveRayTest &test,
        SpatialIndexWorkspace &workspace) const final;
};

#endif
