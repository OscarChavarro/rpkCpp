#include "java/util/ArrayList.txx"
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
patchListBounds(java::ArrayList<Patch *> *patchList, float *boundingBox) {
    BOUNDINGBOX b;

    boundsInit(boundingBox);
    for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
        patchBounds(patchList->get(i), b);
        boundsEnlarge(boundingBox, b);
    }

    return boundingBox;
}

static float *
patchListBoundsVoid(void *patchList, float *boundingBox) {
    // TODO: Check this is working! Must debug and veryfy bounding box values are valid
    java::ArrayList<Patch *> *o = reinterpret_cast<java::ArrayList<Patch *> *>(patchList);
    return patchListBounds(o, boundingBox);
}

/**
Tests whether the Ray intersect the patches in the list. See geometry.h
(GeomDiscretizationIntersect()) for more explanation
*/
RayHit *
patchListIntersect(
    java::ArrayList<Patch *> *patchList,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore)
{
    RayHit *hit = (RayHit *) nullptr;
    for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
        RayHit *h = patchIntersect(patchList->get(i), ray, minimumDistance, maximumDistance, hitFlags, hitStore);
        if ( h ) {
            if ( hitFlags & HIT_ANY ) {
                return h;
            } else {
                hit = h;
            }
        }
    }
    return hit;
}

/**
Tests whether the Ray intersect the patches in the list. See geometry.h
(GeomDiscretizationIntersect()) for more explanation
*/
HITLIST *
patchListAllIntersections(
    HITLIST *hits,
    java::ArrayList<Patch *> *patchList,
    Ray *ray,
    float minimumDistance,
    float maximumDistance,
    int hitFlags)
{
    RayHit hitStore{};
    for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
        Patch *patch = patchList->get(i);
        float maxDistanceCopy = maximumDistance; // Do not modify maximumDistance
        RayHit *hit = patchIntersect(patch, ray, minimumDistance, &maxDistanceCopy, hitFlags, &hitStore);
        if ( hit ) {
            hits = HitListAdd(hits, DuplicateHit(hit));
        }
    }
    return hits;
}

/**
The following routines are needed to make patch lists look like GEOMs. This
is required for shaft culling (see shaft culling.[ch] and clustering (see
cluster.[ch]. In both cases, the patches pointed to should not be
destroyed, only the patches should
*/

static void
patchListDestroy(java::ArrayList<Patch *> *patchList) {
    delete patchList;
}

static void
patchListPrint(FILE *out, java::ArrayList<Patch *> *patchList) {
    fprintf(out, "getPatchList:\n");
    for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
        fprintf(out, "%d ", patchList->get(i)->id);
    }
    fprintf(out, "\nend of getPatchList.\n");
}

static java::ArrayList<Patch *> *
getPatchList(java::ArrayList<Patch *> *patchList) {
    return patchList;
}

static java::ArrayList<Patch *> *
patchListDuplicate(java::ArrayList<Patch *> *patchList) {
    java::ArrayList<Patch *> *newList = new java::ArrayList<Patch *>();;

    for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
        newList->add(patchList->get(i));
    }

    return newList;
}

GEOM_METHODS GLOBAL_skin_patchListGeometryMethods = {
    (float *(*)(void *, float *)) patchListBoundsVoid,
    (void (*)(void *)) patchListDestroy,
    (void (*)(FILE *, void *)) patchListPrint,
    (GeometryListNode *(*)(void *)) nullptr,
    (java::ArrayList<Patch *> *(*)(void *)) getPatchList,
    (RayHit *(*)(void *, Ray *, float, float *, int, RayHit *)) patchListIntersect,
    (HITLIST *(*)(HITLIST *, void *, Ray *, float, float, int)) patchListAllIntersections,
    (void *(*)(void *)) patchListDuplicate
};
