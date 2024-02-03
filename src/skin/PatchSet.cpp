#include "java/util/ArrayList.txx"
#include "skin/bounds.h"
#include "skin/Patch.h"
#include "skin/hitlist.h"
#include "skin/PatchSet.h"

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

/**
DiscretizationIntersect returns nullptr is the ray doesn't hit the discretization
of the object. If the ray hits the object, a hit record is returned containing
information about the intersection point. See geometry.h for more explanation

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
patchListAllIntersections(HITLIST *hits, java::ArrayList<Patch *> *patches, Ray *ray, float minimumDistance, float maximumDistance, int hitFlags) {
    RayHit hitStore;
    for ( int i = 0; patches != nullptr && i < patches->size(); i++ ) {
        Patch *patch = patches->get(i);
        float maxDistanceCopy = maximumDistance; // Do not modify maximumDistance
        RayHit *hit = patchIntersect(patch, ray, minimumDistance, &maxDistanceCopy, hitFlags, &hitStore);
        if ( hit ) {
            hits = HitListAdd(hits, duplicateHit(hit));
        }
    }
    return hits;
}

PatchSet *
patchListDuplicate(PatchSet *patchSet) {
    PatchSet *newPatchSet = nullptr;

    for ( PatchSet *window = patchSet; window != nullptr; window = window->next ) {
        PatchSet *newListNode = (PatchSet *)malloc(sizeof(PatchSet));
        newListNode->patch = window->patch;
        newListNode->next = newPatchSet;
        newPatchSet = newListNode;
    }

    return newPatchSet;
}

extern java::ArrayList<Patch *> *
patchListExportToArrayList(PatchSet *patchSet) {
    java::ArrayList<Patch *> *newList = new java::ArrayList<Patch *>();

    for ( PatchSet *window = patchSet; window != nullptr; window = window->next ) {
        newList->add(0, window->patch);
    }

    return newList;
}
