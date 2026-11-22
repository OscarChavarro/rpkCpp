//= References:                                                             =
//= [WALD2007] Wald, Ingo. "On fast Construction of SAH-based Bounding      =
//=     Volume Hierarchies". Proceedings of the 2007 IEEE Symposium on      =
//=     Interactive Ray Tracing, 2007, pp. 33-40.                           =

#ifndef __BINNED_SAH_SPATIAL_INDEX__
#define __BINNED_SAH_SPATIAL_INDEX__

#include "vsdk/toolkit/common/VSDK.h"
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
class BinnedSahSpatialIndex : public SpatialIndex {
  private:
    BinnedSahHierarchy hierarchy;
    ArrayList<int> unboundedPrimitives;
    ArrayList<char> unbounded;
    int numberOfPrimitives;

  public:
    /**
    @param boxes six values per primitive (lo x, y, z, hi x, y, z), padded by
    the caller; the box of an unbounded primitive is ignored
    @param inUnboundedPrimitives ids of the primitives without a box
    @param maximumLeafSize as in `BinnedSahHierarchy::build`
    */
    BinnedSahSpatialIndex(
        const ArrayList<double> &boxes,
        const ArrayList<int> &inUnboundedPrimitives,
        int maximumLeafSize = BinnedSahHierarchy::DEFAULT_MAXIMUM_LEAF_SIZE);
    virtual ~BinnedSahSpatialIndex();

    virtual int getNumberOfPrimitives() const;
    virtual int getWorkspaceSize() const;
    const BinnedSahHierarchy &getHierarchy() const;
    virtual SpatialQueryResult query(
        const Ray &ray,
        double minimumT,
        double maximumT,
        SpatialQueryMode::Value mode,
        PrimitiveRayTest &test,
        SpatialIndexWorkspace &workspace) const;
};

#endif
