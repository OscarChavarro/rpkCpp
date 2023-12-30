#include "skin/patchlist.h"
#include "skin/geomlist.h"
#include "skin/geom.h"
#include "skin/hitlist.h"

/* this function computes a bounding box for a list of geometries. The bounding box is
 * filled in in 'boundingbox' and a pointer returned. */
float *GeomListBounds(GEOMLIST *geomlist, float *bounds) {
    BoundsInit(bounds);
    ForAllGeoms(geom, geomlist)
                {
                    BoundsEnlarge(bounds, GeomBounds(geom));
                }
    EndForAll;
    return bounds;
}

/* Builds a linear list of PATCHES making up all the GEOMetries in the list, whether
 * they are primitive or not. */
PATCHLIST *
BuildPatchList(GEOMLIST *world, PATCHLIST *patchlist) {
    ForAllGeoms(geom, world)
                {
                    if ( GeomIsAggregate(geom)) {
                        patchlist = BuildPatchList(GeomPrimList(geom), patchlist);
                    } else {
                        patchlist = PatchListMerge(patchlist, GeomPatchList(geom))
                    };
                }
    EndForAll;

    return patchlist;
}

HITREC *
GeomListDiscretisationIntersect(GEOMLIST *geomlist, Ray *ray, float mindist, float *maxdist, int hitflags,
                                HITREC *hitstore) {
    HITREC *h, *hit;

    hit = (HITREC *) nullptr;
    ForAllGeoms(geom, geomlist)
                {
                    if ((h = GeomDiscretisationIntersect(geom, ray, mindist, maxdist, hitflags, hitstore))) {
                        if ( hitflags & HIT_ANY ) {
                            return h;
                        } else {
                            hit = h;
                        }
                    }
                }
    EndForAll;
    return hit;
}

HITLIST *
GeomListAllDiscretisationIntersections(HITLIST *hits, GEOMLIST *geomlist, Ray *ray, float mindist, float maxdist,
                                       int hitflags) {
    ForAllGeoms(geom, geomlist)
                {
                    hits = GeomAllDiscretisationIntersections(hits, geom, ray, mindist, maxdist, hitflags);
                }
    EndForAll;
    return hits;
}
