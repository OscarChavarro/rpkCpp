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
patchListBounds(PatchSet *pl, float *boundingBox) {
    BOUNDINGBOX b;

    boundsInit(boundingBox);
    ForAllPatches(patch, pl)
                {
                    patchBounds(patch, b);
                    boundsEnlarge(boundingBox, b);
                }
    EndForAll;

    return boundingBox;
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

/**
The following routines are needed to make patch lists look like GEOMs. This
is required for shaft culling (see shaft culling.[ch] and clustering (see
cluster.[ch]. In both cases, the patches pointed to should not be
destroyed, only the patches should
*/
static void
patchListDestroy(PatchSet *patchList) {
    PatchSet *listWindow = patchList;
    while ( listWindow != nullptr ) {
        PatchSet *listNode = listWindow->next;
        free(listWindow);
        listWindow = listNode;
    }
}

static void
patchListPrint(FILE *out, PatchSet *patchList) {
    fprintf(out, "getPatchList:\n");
    for ( PatchSet *window = patchList; window != nullptr; window = window->next ) {
        fprintf(out, "%d ", window->patch->id);
    }
    fprintf(out, "\nend of getPatchList.\n");
}

static PatchSet *
getPatchList(PatchSet *patchlist) {
    return patchlist;
}

static PatchSet *
patchListDuplicate(PatchSet *patchList) {
    PatchSet *newList = (PatchSet *) nullptr;
    void *patch;

    while ( (patch = (patchList != nullptr ? (GLOBAL_listHandler = patchList->patch, patchList = patchList->next, GLOBAL_listHandler) : nullptr)) ) {
        PatchSet *newListNode = (PatchSet *)malloc(sizeof(PatchSet));
        newListNode->patch = (PATCH *)patch;
        newListNode->next = newList;
        newList = newListNode;
    }

    return newList;
}

GEOM_METHODS GLOBAL_skin_patchListGeometryMethods = {
        (float *(*)(void *, float *)) patchListBounds,
        (void (*)(void *)) patchListDestroy,
        (void (*)(FILE *, void *)) patchListPrint,
        (GeometryListNode *(*)(void *)) nullptr,
        (PatchSet *(*)(void *)) getPatchList,
        (HITREC *(*)(void *, Ray *, float, float *, int, HITREC *)) patchListIntersect,
        (HITLIST *(*)(HITLIST *, void *, Ray *, float, float, int)) patchListAllIntersections,
        (void *(*)(void *)) patchListDuplicate
};
