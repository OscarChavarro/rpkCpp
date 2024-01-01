#include "common/bounds.h"
#include "skin/PatchSet.h"
#include "skin/patch.h"
#include "skin/hitlist.h"
#include "skin/Geometry.h"

/* computes a bounding box for the given list of PATCHes. The bounding box is
 * filled in 'boundingbox' and a pointer to it returned. */
float *PatchListBounds(PatchSet *pl, float *boundingbox) {
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
 * (geomDiscretizationIntersect()) for more explanation. */
HITREC *PatchListIntersect(PatchSet *pl, Ray *ray, float mindist, float *maxdist, int hitflags, HITREC *hitstore) {
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
PatchListAllIntersections(HITLIST *hits, PatchSet *patches, Ray *ray, float mindist, float maxdist, int hitflags) {
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
 * destroyed, only the PatchSet should. */
static void PatchlistDestroy(PatchSet *patchlist) {
    PatchListDestroy(patchlist);
}

static void PatchPrintId(FILE *out, PATCH *patch) {
    fprintf(out, "%d ", patch->id);
}

static void PatchlistPrint(FILE *out, PatchSet *patchlist) {
    fprintf(out, "getPatchList:\n");
    PatchListIterate1B(patchlist, PatchPrintId, out);
    fprintf(out, "\nend of getPatchList.\n");
}

static PatchSet *PatchlistPatchlist(PatchSet *patchlist) {
    return patchlist;
}

static PatchSet *PatchlistDuplicate(PatchSet *patchlist) {
    return PatchListDuplicate(patchlist);
}

GEOM_METHODS GLOBAL_skin_patchListGeometryMethods = {
        (float *(*)(void *, float *)) PatchListBounds,
        (void (*)(void *)) PatchlistDestroy,
        (void (*)(FILE *, void *)) PatchlistPrint,
        (GeometryListNode *(*)(void *)) nullptr,
        (PatchSet *(*)(void *)) PatchlistPatchlist,
        (HITREC *(*)(void *, Ray *, float, float *, int, HITREC *)) PatchListIntersect,
        (HITLIST *(*)(HITLIST *, void *, Ray *, float, float, int)) PatchListAllIntersections,
        (void *(*)(void *)) PatchlistDuplicate
};
