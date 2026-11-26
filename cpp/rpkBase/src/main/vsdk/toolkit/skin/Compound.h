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
        Ray *ray,
        double minimumDistance,
        double *maximumDistance,
        int hitFlags,
        RpkRayHit *hitStore,
        const RayExclusionSet &exclusions) const override;
};

#endif
