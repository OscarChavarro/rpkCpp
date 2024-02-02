#ifndef __COMPOUND__
#define __COMPOUND__

#include "skin/geomlist.h"

class GEOM_METHODS;

class Compound /*: public Geometry*/ {
public:
    GeometryListNode children;
};

extern GEOM_METHODS GLOBAL_skin_compoundGeometryMethods;

extern Compound *compoundCreate(GeometryListNode *geometryList);

extern RayHit *
compoundDiscretizationIntersect(
    Compound *obj,
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
compoundBounds(Compound *obj, float *boundingBox);

extern HITLIST *
compoundAllDiscretizationIntersections(
        HITLIST *hits,
        Compound *obj,
        Ray *ray,
        float minimumDistance,
        float maximumDistance,
        int hitFlags);

#endif
