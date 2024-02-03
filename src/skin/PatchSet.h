#ifndef __PATCH_LIST__
#define __PATCH_LIST__

#include "java/util/ArrayList.h"
#include "common/Ray.h"
#include "material/hit.h"
#include "skin/Patch.h"
#include "skin/Geometry.h"

class PatchSet {
  public:
    Patch *patch;
    PatchSet *next;
};

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
patchListExportToArrayList(PatchSet *patchSet);

extern PatchSet *
patchListDuplicate(PatchSet *patchSet);

#endif
