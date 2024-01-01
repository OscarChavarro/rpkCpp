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

#define GeomListCreate (GeometryListNode *)ListCreate

#define GeomListAdd(geomlist, geom)    \
        (GeometryListNode *)ListAdd((LIST *)geomlist, (void *)geom)

#define GeomListCount(geomlist) \
        ListCount((LIST *)geomlist)

#define GeomListIterate(geomlist, proc) \
        ListIterate((LIST *)geomlist, (void (*)(void *))proc)

#define GeomListIterate1A(geomlist, proc, data) \
        ListIterate1A((LIST *)geomlist, (void (*)(void *, void *))proc, (void *)data)

#define GeomListIterate1B(geomlist, proc, data) \
        ListIterate1B((LIST *)geomlist, (void (*)(void *, void *))proc, (void *)data)

#define GeomListDestroy(geomlist) \
        ListDestroy((LIST *)geomlist)

#define ForAllGeoms(geom, geomlist)    ForAllInList(Geometry, geom, geomlist)

extern float *GeomListBounds(GeometryListNode *geomlist, float *boundingbox);
extern PatchSet *BuildPatchList(GeometryListNode *world, PatchSet *patchlist);

extern HITREC *
GeomListDiscretizationIntersect(
    GeometryListNode *geomlist,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    HITREC *hitStore);

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
