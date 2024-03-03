#ifndef __COMPOUND__
#define __COMPOUND__

#include "java/util/ArrayList.h"
#include "skin/Geometry.h"

class Compound : public Geometry {
public:
    java::ArrayList<Geometry *> *children;

    explicit Compound(java::ArrayList<Geometry *> *geometryList);
    virtual ~Compound();

    RayHit *
    discretizationIntersect(
        Ray *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore) const;

};

#endif
