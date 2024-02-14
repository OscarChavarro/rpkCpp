#ifndef __GEOMETRY_LIST__
#define __GEOMETRY_LIST__

#include "java/util/ArrayList.h"
#include "common/Ray.h"
#include "material/hit.h"
#include "skin/Geometry.h"

extern float *geometryListBounds(java::ArrayList<Geometry *> *geometryList, float *boundingBox);

extern RayHit *
geometryListDiscretizationIntersect(
    java::ArrayList<Geometry *> *geometryList,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore);

#endif
