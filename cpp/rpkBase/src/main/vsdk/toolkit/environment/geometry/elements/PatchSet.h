#ifndef __PATCH_SET__
#define __PATCH_SET__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"

class PatchSet final : public RpkGeometry {
  private:
    java::ArrayList<Patch *> *patchList;
    bool memoryPoolManaged;

  public:
    explicit PatchSet(const java::ArrayList<Patch *> *input);
    // patchBounds: the union of the patches' bounds, which the caller already
    // has (for instance from the scene's FaceBoundsTable); the set is then
    // bounded exactly as by the first constructor, without visiting vertices
    PatchSet(const java::ArrayList<Patch *> *input, const RpkAxisAlignedBoundingBox &patchBounds);
    ~PatchSet() final;

    RpkRayHit *
    discretizationIntersect(
        RpkRay *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RpkRayHit *hitStore,
        const RayExclusionSet &exclusions) const final;

    java::ArrayList<Patch *> *getPatchList() const;
    bool isMemoryPoolManaged() const;
    void setMemoryPoolManaged(bool value);
};

#endif
