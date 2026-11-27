#ifndef __COMPOUND__
#define __COMPOUND__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"

class Compound: public RpkGeometry{ public:
    ArrayList<RpkGeometry *> *children;

    explicit Compound(ArrayList<RpkGeometry *> *geometryList);
    ~Compound();

    RpkRayHit *
    discretizationIntersect( Ray *ray, double minimumDistance, double *maximumDistance, int hitFlags, RpkRayHit *hitStore, const RayExclusionSet &exclusions) const;
};

#endif
