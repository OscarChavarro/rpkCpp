#include "common/bounds.h"
#include "skin/patchlist.h"
#include "skin/patch.h"
#include "skin/hitlist.h"
#include "skin/geom.h"

/* computes a bounding box for the given list of PATCHes. The bounding box is
 * filled in 'boundingbox' and a pointer to it returned. */
float *PatchListBounds(PATCHLIST *pl, float *boundingbox) {
    BOUNDINGBOX b;

    BoundsInit(boundingbox);
    ForAllPatches(patch, pl)
                {
                    PatchBounds(patch, b);
                    BoundsEnlarge(boundingbox, b);
                }
    EndForAll;

    return boundingbox;
}

/* Tests whether the Ray intersect the PATCHes in the list. See geom.h
 * (GeomDiscretisationIntersect()) for more explanation. */
HITREC *PatchListIntersect(PATCHLIST *pl, Ray *ray, float mindist, float *maxdist, int hitflags, HITREC *hitstore) {
    HITREC *hit = (HITREC *) nullptr;
    ForAllPatches(P, pl)
                {
                    HITREC *h = PatchIntersect(P, ray, mindist, maxdist, hitflags, hitstore);
                    if ( h ) {
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
PatchListAllIntersections(HITLIST *hits, PATCHLIST *patches, Ray *ray, float mindist, float maxdist, int hitflags) {
    HITREC hitstore;
    ForAllPatches(P, patches)
                {
                    float tmax = maxdist;    /* do not modify maxdist */
                    HITREC *hit = PatchIntersect(P, ray, mindist, &tmax, hitflags, &hitstore);
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
 * destroyed, only the PATCHLIST should. */
static void PatchlistDestroy(PATCHLIST *patchlist) {
    PatchListDestroy(patchlist);
}

static void PatchPrintId(FILE *out, PATCH *patch) {
    fprintf(out, "%d ", patch->id);
}

static void PatchlistPrint(FILE *out, PATCHLIST *patchlist) {
    fprintf(out, "patchlist:\n");
    PatchListIterate1B(patchlist, PatchPrintId, out);
    fprintf(out, "\nend of patchlist.\n");
}

static PATCHLIST *PatchlistPatchlist(PATCHLIST *patchlist) {
    return patchlist;
}

static PATCHLIST *PatchlistDuplicate(PATCHLIST *patchlist) {
    return PatchListDuplicate(patchlist);
}

GEOM_METHODS GLOBAL_skin_patchListGeometryMethods = {
        (float *(*)(void *, float *)) PatchListBounds,
        (void (*)(void *)) PatchlistDestroy,
        (void (*)(FILE *, void *)) PatchlistPrint,
        (GEOMLIST *(*)(void *)) nullptr,
        (PATCHLIST *(*)(void *)) PatchlistPatchlist,
        (HITREC *(*)(void *, Ray *, float, float *, int, HITREC *)) PatchListIntersect,
        (HITLIST *(*)(HITLIST *, void *, Ray *, float, float, int)) PatchListAllIntersections,
        (void *(*)(void *)) PatchlistDuplicate
};
