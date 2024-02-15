#ifndef __COMPOUND__
#define __COMPOUND__

#include "java/util/ArrayList.h"
#include "skin/geomlist.h"

class Compound : public Geometry {
public:
    java::ArrayList<Geometry *> *children;

    explicit Compound(java::ArrayList<Geometry *> *geometryList);

    HITLIST *
    allDiscretizationIntersections(
        HITLIST *hits,
        Ray *ray,
        float minimumDistance,
        float maximumDistance,
        int hitFlags) const;

    RayHit *
    discretizationIntersect(
        Ray *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore);

};

#endif
