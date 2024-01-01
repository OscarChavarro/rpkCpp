#include "skin/PatchSet.h"
#include "skin/geomlist.h"
#include "skin/Geometry.h"
#include "skin/hitlist.h"

/* this function computes a bounding box for a list of geometries. The bounding box is
 * filled in in 'boundingbox' and a pointer returned. */
float *GeomListBounds(GeometryListNode *geomlist, float *bounds) {
    BoundsInit(bounds);
    ForAllGeoms(geom, geomlist)
                {
                    BoundsEnlarge(bounds, geomBounds(geom));
                }
    EndForAll;
    return bounds;
}

/* Builds a linear list of PATCHES making up all the GEOMetries in the list, whether
 * they are primitive or not. */
PatchSet *
BuildPatchList(GeometryListNode *world, PatchSet *patchlist) {
    ForAllGeoms(geom, world)
                {
                    if ( geomIsAggregate(geom)) {
                        patchlist = BuildPatchList(geomPrimList(geom), patchlist);
                    } else {
                        patchlist = PatchListMerge(patchlist, geomPatchList(geom))
                    };
                }
    EndForAll;

    return patchlist;
}

HITREC *
GeomListDiscretizationIntersect(GeometryListNode *geomlist, Ray *ray, float minimumDistance, float *maximumDistance, int hitFlags,
                                HITREC *hitStore) {
    HITREC *h, *hit;

    hit = (HITREC *) nullptr;
    ForAllGeoms(geom, geomlist)
                {
                    if ((h = geomDiscretizationIntersect(geom, ray, minimumDistance, maximumDistance, hitFlags, hitStore))) {
                        if ( hitFlags & HIT_ANY ) {
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
geomListAllDiscretizationIntersections(HITLIST *hits, GeometryListNode *geometryList, Ray *ray, float minimumDistance, float maximumDistance,
                                       int hitFlags) {
    ForAllGeoms(geom, geometryList)
                {
                    hits = geomAllDiscretizationIntersections(hits, geom, ray, minimumDistance, maximumDistance, hitFlags);
                }
    EndForAll;
    return hits;
}
