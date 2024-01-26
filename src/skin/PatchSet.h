#ifndef __PATCH_LIST__
#define __PATCH_LIST__

#include "java/util/ArrayList.h"
#include "common/dataStructures/List.h"
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

extern float *patchListBounds(PatchSet *pl, float *boundingBox);

extern RayHit *
patchListIntersect(
        PatchSet *patchList,
        Ray *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore);

extern HITLIST *
patchListAllIntersections(
    HITLIST *hits,
    PatchSet *patches,
    Ray *ray,
    float minimumDistance,
    float maximumDistance,
    int hitFlags);

extern java::ArrayList<Patch *> *
convertPatchSetToPatchList(PatchSet *patchSet);

#include "skin/Geometry.h"

#endif
