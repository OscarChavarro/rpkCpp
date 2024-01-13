#ifndef __GEOMETRY_LIST__
#define __GEOMETRY_LIST__

#include "java/util/ArrayList.h"
#include "common/Ray.h"
#include "material/hit.h"
#include "skin/hitlist.h"

#include "common/dataStructures/List.h"

class Geometry;

class GeometryListNode {
  public:
    Geometry *geom;
    GeometryListNode *next;
};

#define GeomListCount(geomlist) \
        ListCount((LIST *)geomlist)

#define GeomListIterate(geomlist, proc) \
        ListIterate((LIST *)geomlist, (void (*)(void *))proc)

#define GeomListDestroy(geomlist) \
        ListDestroy((LIST *)geomlist)

#define ForAllGeoms(geom, geomlist)    ForAllInList(Geometry, geom, geomlist)

extern GeometryListNode *
geometryListAdd(GeometryListNode *geometryList, Geometry *geometry);

extern float *
geometryListBounds(GeometryListNode *geometryList, float *boundingBox);

void
buildPatchList(GeometryListNode *geometryList, java::ArrayList<Patch*> *patchList);

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
