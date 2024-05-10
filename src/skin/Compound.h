#ifndef __COMPOUND__
#define __COMPOUND__

#include "java/util/ArrayList.h"
#include "skin/Geometry.h"

class Compound final : public Geometry {
  public:
    java::ArrayList<Geometry *> *children;

    explicit Compound(java::ArrayList<Geometry *> *geometryList);
    ~Compound() final;

    RayHit *
    discretizationIntersect(
        Ray *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore) const final;
};

#endif
