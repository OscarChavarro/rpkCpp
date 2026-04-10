#ifndef __COMPOUND__
#define __COMPOUND__

#include "java/util/ArrayList.h"
#include "skin/Geometry.h"

class Compound: public Geometry{ public:
    ArrayList<Geometry *> *children;

    explicit Compound(ArrayList<Geometry *> *geometryList);
    ~Compound();

    RayHit *
    discretizationIntersect( Ray *ray, float minimumDistance, float *maximumDistance, int hitFlags, RayHit *hitStore) const;
};

#endif
