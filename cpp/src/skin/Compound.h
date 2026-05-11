#ifndef COMPOUND__
#define COMPOUND__

#include "java/util/ArrayList.h"
#include "skin/Geometry.h"

class Compound final : public Geometry {
  public:
    java::ArrayList<Geometry *> *children;

    explicit Compound(java::ArrayList<Geometry *> *geometryList);
    ~Compound() override;

    RayHit *
    discretizationIntersect(
        Ray *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore) const override;
};

#endif
