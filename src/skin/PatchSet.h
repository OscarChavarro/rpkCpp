#ifndef __PATCH_LIST__
#define __PATCH_LIST__

#include "java/util/ArrayList.h"
#include "material/hit.h"
#include "skin/Patch.h"
#include "common/Ray.h"

class GEOM_METHODS;

class PatchSet {
  public:
    Patch *patch;
    PatchSet *next;
};

class HITLIST;

// Note: Should create a Geometry class for a set of patches!
extern GEOM_METHODS GLOBAL_skin_patchListGeometryMethods;

extern float *patchListBounds(java::ArrayList<Patch *> *patchList, float *boundingBox);

extern RayHit *
patchListIntersect(
    java::ArrayList<Patch *> *patchList,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore);

extern HITLIST *
patchListAllIntersections(
    HITLIST *hits,
    java::ArrayList<Patch *> *patches,
    Ray *ray,
    float minimumDistance,
    float maximumDistance,
    int hitFlags);

extern java::ArrayList<Patch *> *
patchListExportToArrayList(PatchSet *patches);

#include "skin/Geometry.h"

#endif
