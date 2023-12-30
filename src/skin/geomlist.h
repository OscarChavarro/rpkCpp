/* geomlist.h: linear lists of GEOMetries */

#ifndef _GEOMLIST_H_
#define _GEOMLIST_H_

#include "material/hit.h"
#include "common/dataStructures/List.h"
#include "skin/geom.h"
#include "skin/hitlist.h"

/* same layout as LIST in dataStructures/List.h, so the same generic functions for
 * operating on lists can be used for lists of geometries */
class GEOMLIST {
  public:
    GEOM *geom;
    GEOMLIST *next;
};

class PATCHLIST;

#define GeomListCreate    (GEOMLIST *)ListCreate

#define GeomListAdd(geomlist, geom)    \
        (GEOMLIST *)ListAdd((LIST *)geomlist, (void *)geom)

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

#define ForAllGeoms(geom, geomlist)    ForAllInList(GEOM, geom, geomlist)

/* this function computes a bounding box for a list of geometries. The bounding box is
 * filled in in 'boundingbox' and a pointer returned. */
extern float *GeomListBounds(GEOMLIST *geomlist, float *boundingbox);

/* Builds a linear list of PATCHES making up all the GEOMetries in the list, whether
 * they are primitive or not. */
extern PATCHLIST *BuildPatchList(GEOMLIST *world, PATCHLIST *patchlist);

/* Tests if the Ray intersects the discretisation of the GEOMetries in the list.
 * See geom.h (GeomDiscretisationIntersect()) */
extern HITREC *
GeomListDiscretisationIntersect(GEOMLIST *geomlist, Ray *ray, float mindist, float *maxdist, int hitflags,
                                HITREC *hitstore);
extern HITLIST *
GeomListAllDiscretisationIntersections(HITLIST *hits, GEOMLIST *geomlist, Ray *ray, float mindist, float maxdist,
                                       int hitflags);

#include "skin/patchlist.h"

#endif
