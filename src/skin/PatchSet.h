#ifndef __PATCH_LIST__
#define __PATCH_LIST__

#include "common/dataStructures/List.h"
#include "material/hit.h"
#include "skin/Patch.h"
#include "common/Ray.h"

class GEOM_METHODS;

class PatchSet {
  public:
    PATCH *patch;
    PatchSet *next;
};

class HITLIST;

#define ForAllPatches(P, patches) ForAllInList(PATCH, P, patches)

// Note: Should create a Geometry class for a set of patches!
extern GEOM_METHODS GLOBAL_skin_patchListGeometryMethods;

extern float *patchListBounds(PatchSet *pl, float *boundingBox);

extern HITREC *
patchListIntersect(
    PatchSet *patchList,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    HITREC *hitStore);

extern HITLIST *
patchListAllIntersections(
    HITLIST *hits,
    PatchSet *patches,
    Ray *ray,
    float minimumDistance,
    float maximumDistance,
    int hitFlags);

#include "skin/Geometry.h"

#endif
