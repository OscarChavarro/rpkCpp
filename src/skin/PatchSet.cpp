#include "skin/bounds.h"
#include "skin/PatchSet.h"
#include "skin/Patch.h"
#include "skin/hitlist.h"
#include "skin/Geometry.h"

/**
Computes a bounding box for the given list of patches. The bounding box is
filled in 'bounding box' and a pointer to it returned
*/
float *
patchListBounds(PatchSet *pl, float *boundingbox) {
    BOUNDINGBOX b;

    BoundsInit(boundingbox);
    ForAllPatches(patch, pl)
                {
                    patchBounds(patch, b);
                    BoundsEnlarge(boundingbox, b);
                }
    EndForAll;

    return boundingbox;
}

/**
Tests whether the Ray intersect the patches in the list. See geometry.h
(GeomDiscretizationIntersect()) for more explanation
*/
HITREC *
patchListIntersect(PatchSet *patchList, Ray *ray, float minimumDistance, float *maximumDistance, int hitFlags, HITREC *hitStore) {
    HITREC *hit = (HITREC *) nullptr;
    ForAllPatches(P, patchList)
                {
                    HITREC *h = patchIntersect(P, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
                    if ( h ) {
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

/**
Tests whether the Ray intersect the patches in the list. See geometry.h
(GeomDiscretizationIntersect()) for more explanation
*/
HITLIST *
patchListAllIntersections(HITLIST *hits, PatchSet *patches, Ray *ray, float minimumDistance, float maximumDistance, int hitFlags) {
    HITREC hitstore;
    ForAllPatches(P, patches)
                {
                    float tmax = maximumDistance;    /* do not modify maximumDistance */
                    HITREC *hit = patchIntersect(P, ray, minimumDistance, &tmax, hitFlags, &hitstore);
                    if ( hit ) {
                        hits = HitListAdd(hits, DuplicateHit(hit));
                    }
                }
    EndForAll;
    return hits;
}

/* The following routines are needed to make patchlists look like GEOMs. This 
 * is required for shaft culling (see shaftculling.[ch] and clustering (see
 * cluster.[ch]. In both cases, the PATCHes pointed to should not be
 * destroyed, only the PatchSet should. */
static void
PatchlistDestroy(PatchSet *patchlist) {
    PatchListDestroy(patchlist);
}

static void
PatchPrintId(FILE *out, PATCH *patch) {
    fprintf(out, "%d ", patch->id);
}

static void
PatchlistPrint(FILE *out, PatchSet *patchlist) {
    fprintf(out, "getPatchList:\n");
    PatchListIterate1B(patchlist, PatchPrintId, out);
    fprintf(out, "\nend of getPatchList.\n");
}

static PatchSet *
PatchlistPatchlist(PatchSet *patchlist) {
    return patchlist;
}

static PatchSet *
PatchlistDuplicate(PatchSet *patchlist) {
    return PatchListDuplicate(patchlist);
}

GEOM_METHODS GLOBAL_skin_patchListGeometryMethods = {
        (float *(*)(void *, float *)) patchListBounds,
        (void (*)(void *)) PatchlistDestroy,
        (void (*)(FILE *, void *)) PatchlistPrint,
        (GeometryListNode *(*)(void *)) nullptr,
        (PatchSet *(*)(void *)) PatchlistPatchlist,
        (HITREC *(*)(void *, Ray *, float, float *, int, HITREC *)) patchListIntersect,
        (HITLIST *(*)(HITLIST *, void *, Ray *, float, float, int)) patchListAllIntersections,
        (void *(*)(void *)) PatchlistDuplicate
};
