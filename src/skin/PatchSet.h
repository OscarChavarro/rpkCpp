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

#define PatchListIterate(patchlist, proc) \
        ListIterate((LIST *)patchlist, (void (*)(void *))proc)

#define PatchListIterate1B(patchlist, proc, data) \
        ListIterate1B((LIST *)patchlist, (void (*)(void *, void *))proc, (void *)data)

#define PatchListDestroy(patchlist) \
        ListDestroy((LIST *)patchlist)

#define ForAllPatches(P, patches) ForAllInList(PATCH, P, patches)

// Note: Should create a Geometry class for a set of patches!
extern GEOM_METHODS GLOBAL_skin_patchListGeometryMethods;

extern float *patchListBounds(PatchSet *pl, float *boundingbox);

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
