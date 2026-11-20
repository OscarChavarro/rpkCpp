#ifndef PATCH_LIST__
#define PATCH_LIST__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"

class PatchSet final : public RpkGeometry {
  private:
    java::ArrayList<Patch *> *patchList;
    bool memoryPoolManaged;

  public:
    explicit PatchSet(const java::ArrayList<Patch *> *input);
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
