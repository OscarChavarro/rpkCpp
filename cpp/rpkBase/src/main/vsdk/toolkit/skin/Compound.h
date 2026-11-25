#ifndef __COMPOUND__
#define __COMPOUND__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"

class Compound final : public RpkGeometry {
  public:
    java::ArrayList<RpkGeometry *> *children;

    explicit Compound(java::ArrayList<RpkGeometry *> *geometryList);
    ~Compound() override;

    RpkRayHit *
    discretizationIntersect(
        RpkRay *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RpkRayHit *hitStore,
        const RayExclusionSet &exclusions) const override;
};

#endif
