#ifndef __COMPOUND__
#define __COMPOUND__

#include "skin/geomlist.h"

class Compound : public Geometry {
public:
    GeometryListNode children;
};

extern Compound *compoundCreate(GeometryListNode *geometryList);

extern RayHit *
compoundDiscretizationIntersect(
    GeometryListNode *obj,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore);

extern RayHit *
aggregationDiscretizationIntersect(
    GeometryListNode *obj,
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
        GeometryListNode *obj,
        Ray *ray,
        float minimumDistance,
        float maximumDistance,
        int hitFlags);

#endif
