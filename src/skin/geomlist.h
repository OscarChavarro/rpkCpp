#ifndef __GEOMETRY_LIST__
#define __GEOMETRY_LIST__

#include "java/util/ArrayList.h"
#include "common/dataStructures/List.h"
#include "common/Ray.h"
#include "material/hit.h"
#include "skin/hitlist.h"

class Geometry;

class GeometryListNode {
  public:
    Geometry *geometry;
    GeometryListNode *next;
};

extern float *geometryListBounds(java::ArrayList<Geometry *> *geometryList, float *boundingBox);
extern void buildPatchList(java::ArrayList<Geometry *> *geometryList, java::ArrayList<Patch *> *patchList);

extern RayHit *
geometryListDiscretizationIntersect(
    java::ArrayList<Geometry *> *geometryList,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore);

extern HITLIST *
geometryListAllDiscretizationIntersections(
    HITLIST *hits,
    java::ArrayList<Geometry *> *geometryList,
    Ray *ray,
    float minimumDistance,
    float maximumDistance,
    int hitFlags);

#include "skin/Geometry.h"

#endif
