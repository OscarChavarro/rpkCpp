#include "java/util/ArrayList.txx"
#include "skin/Geometry.h"
#include "skin/geomlist.h"

/**
This function computes a bounding box for a list of geometries
*/
float *
geometryListBounds(java::ArrayList<Geometry *> *geometryList, float *boundingBox) {
    boundsInit(boundingBox);
    for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
        boundsEnlarge(boundingBox, geometryList->get(i)->bounds);
    }
    return boundingBox;
}

/**
Builds a linear list of patches making up all the geometries in the list, whether
they are primitive or not
*/
void
buildPatchList(java::ArrayList<Geometry *> *geometryList, java::ArrayList<Patch *> *patchList) {
    if ( geometryList == nullptr ) {
        return;
    }

    for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
        Geometry *geometry = geometryList->get(i);
        if ( geomIsAggregate(geometry) ) {
            java::ArrayList<Geometry *> *subList = geomPrimListReference(geometry);
            buildPatchList(subList, patchList);
        } else {
            java::ArrayList<Patch *> *list2 = geomPatchArrayList(geometry);

            for ( int j = 0; list2 != nullptr && j < list2->size(); j++ ) {
                Patch *patch = list2->get(j);
                if ( patch != nullptr ) {
                    patchList->add(0, patch);
                }
            }
        }
    }
}

RayHit *
geometryListDiscretizationIntersect(
    java::ArrayList<Geometry *> *geometryList,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore)
{
    RayHit *hit = nullptr;

    for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
        RayHit *h = geomDiscretizationIntersect(geometryList->get(i), ray, minimumDistance, maximumDistance, hitFlags, hitStore);
        if ( h != nullptr ) {
            if ( hitFlags & HIT_ANY ) {
                return h;
            } else {
                hit = h;
            }
        }
    }
    return hit;
}

HITLIST *
geometryListAllDiscretizationIntersections(
    HITLIST *hits,
    java::ArrayList<Geometry *> *geometryList,
    Ray *ray,
    float minimumDistance,
    float maximumDistance,
    int hitFlags)
{
    for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
        hits = geomAllDiscretizationIntersections(hits, geometryList->get(i), ray, minimumDistance, maximumDistance, hitFlags);
    }
    return hits;
}
