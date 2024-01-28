#ifndef __GEOMETRY_LIST__
#define __GEOMETRY_LIST__

#include "java/util/ArrayList.h"
#include "common/Ray.h"
#include "material/hit.h"
#include "skin/hitlist.h"

#include "common/dataStructures/List.h"

class Geometry;
class PatchSet;

class GeometryListNode {
  public:
    Geometry *geom;
    GeometryListNode *next;
};

extern GeometryListNode *
geometryListAdd(GeometryListNode *geometryList, Geometry *geometry);

extern float *
geometryListBounds(GeometryListNode *geometryList, float *boundingBox);

extern PatchSet *
buildPatchList(GeometryListNode *geometryList, PatchSet *patchList);

extern RayHit *
geometryListDiscretizationIntersect(
        GeometryListNode *geometryList,
        Ray *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore);

extern HITLIST *
geomListAllDiscretizationIntersections(
    HITLIST *hits,
    GeometryListNode *geometryList,
    Ray *ray,
    float minimumDistance,
    float maximumDistance,
    int hitFlags);

#include "skin/Geometry.h"
#include "skin/PatchSet.h"

#endif
