#ifndef __BINNED_SAH_SPATIAL_INDEX__
#define __BINNED_SAH_SPATIAL_INDEX__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/BinnedSahHierarchy.h"
#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialIndex.h"

/**
`SpatialIndex` over a `BinnedSahHierarchy` ([WALD2007]): the binned SAH
builder and its nearest-first traversal, through
`BinnedSahHierarchy::traverse`, which can stop at the first accepted hit and
counts into the workspace instead of `RaytraceStatistics`.

Unbounded primitives are kept out of the hierarchy, on an explicit list that
every query tests first, in list order. Candidates of the hierarchy come in
traversal order, so with hits at equal parameters the accepted one depends on
the caller's tie rule; ANY_HIT stops at the first accepted candidate.
*/
class BinnedSahSpatialIndex final : public SpatialIndex {
  private:
    BinnedSahHierarchy hierarchy;
    java::ArrayList<int> unboundedPrimitives;
    java::ArrayList<char> unbounded;
    int numberOfPrimitives;

  public:
    /**
    @param boxes six values per primitive (lo x, y, z, hi x, y, z), padded by
    the caller; the box of an unbounded primitive is ignored
    @param inUnboundedPrimitives ids of the primitives without a box
    @param maximumLeafSize as in `BinnedSahHierarchy::build`
    */
    BinnedSahSpatialIndex(
        const java::ArrayList<double> &boxes,
        const java::ArrayList<int> &inUnboundedPrimitives,
        int maximumLeafSize = BinnedSahHierarchy::DEFAULT_MAXIMUM_LEAF_SIZE);
    ~BinnedSahSpatialIndex() final;

    int getNumberOfPrimitives() const final;
    int getWorkspaceSize() const final;
    const BinnedSahHierarchy &getHierarchy() const;
    SpatialQueryResult query(
        const Ray &ray,
        double minimumT,
        double maximumT,
        SpatialQueryMode mode,
        PrimitiveRayTest &test,
        SpatialIndexWorkspace &workspace) const final;
};

#endif
