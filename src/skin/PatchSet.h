#ifndef _PATCHLIST_H_
#define _PATCHLIST_H_

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

#define PatchListCreate    (PatchSet *)ListCreate

#define PatchListAdd(patchlist, patch)    \
        (PatchSet *)ListAdd((LIST *)patchlist, (void *)patch)

#define PatchListDuplicate(patchlist) \
        (PatchSet *)ListDuplicate((LIST *)patchlist)

#define PatchListMerge(patchlist1, patchlist2) \
        (PatchSet *)ListMerge((LIST *)patchlist1, (LIST *)patchlist2);

#define PatchListIterate(patchlist, proc) \
        ListIterate((LIST *)patchlist, (void (*)(void *))proc)

#define PatchListIterate1A(patchlist, proc, data) \
        ListIterate1A((LIST *)patchlist, (void (*)(void *, void *))proc, (void *)data)

#define PatchListIterate1B(patchlist, proc, data) \
        ListIterate1B((LIST *)patchlist, (void (*)(void *, void *))proc, (void *)data)

#define PatchListDestroy(patchlist) \
        ListDestroy((LIST *)patchlist)

#define ForAllPatches(P, patches) ForAllInList(PATCH, P, patches)

/* Computes a bounding box for the given list of PATCHes. The bounding box is
 * filled in in 'boundingbox' and a pointer to it returned. */
extern float *PatchListBounds(PatchSet *pl, float *boundingbox);

/* Tests whether the Ray intersect the PATCHes in the list. See geom.h
 * (geomDiscretizationIntersect()) for more explanation. */
extern HITREC *
PatchListIntersect(PatchSet *pl, Ray *ray, float mindist, float *maxdist, int hitflags, HITREC *hitstore);

/* similar, but adds all found intersections to the hitlist */
extern HITLIST *
PatchListAllIntersections(HITLIST *hits, PatchSet *patches, Ray *ray, float mindist, float maxdist,
                          int hitflags);

extern GEOM_METHODS GLOBAL_skin_patchListGeometryMethods;

#include "skin/Geometry.h"

#endif
