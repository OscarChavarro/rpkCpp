#ifndef __COMPOUND__
#define __COMPOUND__

#include "java/util/ArrayList.h"
#include "skin/geomlist.h"

class Compound : public Geometry {
public:
    GeometryListNode *children;
};

extern Compound *compoundCreate(GeometryListNode *geometryList);

extern RayHit *
compoundDiscretizationIntersect(
    Compound *compound,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore);

extern float *
compoundBounds(GeometryListNode *obj, float *boundingBox);

extern HITLIST *
compoundAllDiscretizationIntersections(
    HITLIST *hits,
    Compound *compound,
    Ray *ray,
    float minimumDistance,
    float maximumDistance,
    int hitFlags);

#endif
